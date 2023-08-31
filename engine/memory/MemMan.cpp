#include "MemMan.h"
#include <new>
#include <string.h>
#include <assert.h>
#include "../dev/print.h"
#include "../common/file_utils.h"
#include "Pool.h"
#include "FrameStack.h"
#include "File.h"
#include "Gobj.h"
#include "FSA.h"
#include "File.h"
#include "FreeList.h"
#include "CharKeys.h"
#include "GLTFLoader3.h"
#include "GLTFLoader4.h"

#if DEBUG
constexpr static bool ShowMemManBGFXDbg = false;
#endif // DEBUG

size_t MemMan::BlockInfo::paddingSize() const {
    return _padding;
}

size_t MemMan::BlockInfo::dataSize() const {
    return _dataSize;
}

size_t MemMan::BlockInfo::blockSize() const {
    return _padding + BlockInfoSize + _dataSize;
}

MemBlockType MemMan::BlockInfo::type() const {
    return _type;
}

byte_t const * MemMan::BlockInfo::data() const {
    return (byte_t *)this + BlockInfoSize;
}
byte_t * MemMan::BlockInfo::data() {
    return (byte_t *)this + BlockInfoSize;
}

byte_t const * MemMan::BlockInfo::calcUnalignedDataLoc() const {
    return data() - _padding;
}

size_t MemMan::BlockInfo::calcAlignPaddingSize(size_t align) const {
    #if DEBUG
    assert(isValid() && "Block not valid.");
    #endif // DEBUG

    // block may be aligned, but treat it as though its not to get potential
    // new align location. this is then used to calc new padding size.
    // using mem_utils function.
    return alignPadding((size_t)calcUnalignedDataLoc(), align);

    // if (align == 0) return 0;
    // size_t dataPtrAsNum = (size_t)calcUnalignedDataLoc();
    // size_t remainder = dataPtrAsNum % align;
    // return (remainder) ? align - remainder : 0;
}

size_t MemMan::BlockInfo::calcAlignDataSize(size_t align) const {
    // there may be padding already set, but this should return what the value
    // would be if re-aligned to parameter align
    return (_padding + _dataSize) - calcAlignPaddingSize(align);
}

bool MemMan::BlockInfo::isAligned(size_t align) const {
    return (_padding == calcAlignPaddingSize(align));
}

byte_t const * MemMan::BlockInfo::basePtr() const {
    return data() - BlockInfoSize - _padding;
}
byte_t * MemMan::BlockInfo::basePtr() {
    return data() - BlockInfoSize - _padding;
}

bool MemMan::BlockInfo::contains(void * ptr, size_t size) const {
    byte_t const * bptr = (byte_t const *)ptr;
    byte_t const * data = this->data();
    return (data <= bptr && bptr + size <= data + _dataSize);
}

bool MemMan::BlockInfo::isValid() const {
    return (
        #if DEBUG
        // magic string valid
        (memcmp(&BlockMagicString, _debug_magic, 4) == 0) &&
        #endif // DEBUG

        // _type valid
        ((int)_type >= 0 && (int)_type <= MEM_BLOCK_GENERIC) &&

        // if free, padding should be 0
        (_type != MEM_BLOCK_FREE || (_type == MEM_BLOCK_FREE && _padding == 0)) &&

        // expected data size
        (_dataSize < 0xffffffff) &&

        #if DEBUG
        // no unlikely pointers (indicates debug-only data overflow)
        ((size_t)_next != 0xfefefefefefefefe && (size_t)_prev != 0xfefefefefefefefe) &&
        ((size_t)_next != 0xefefefefefefefef && (size_t)_prev != 0xefefefefefefefef) &&
        ((size_t)_next != 0xffffffffffffffff && (size_t)_prev != 0xffffffffffffffff) &&
        #endif // DEBUG

        // probable padding size
        (_padding <= 256) &&

        true
    );
}

#if DEBUG
void MemMan::BlockInfo::print(size_t index) const {
    if (index == SIZE_MAX) {
        index = _debug_index;
    }
    printl("BLOCK %zu %s (%p, data: %p)", index, memBlockTypeStr(_type), this, data());
    printl("    data size: %zu", _dataSize);
    printl("    padding: %zu", _padding);
    printl("    prev: %p", _prev);
    printl("    next: %p", _next);
    printl("    debug index: %zu", _debug_index);
    printl("    magic string: %.*s", 8, _debug_magic);
}
#endif // DEBUG

void MemMan::init(EngineSetup const & setup, FrameStack ** frameStack) {
    if (setup.memManSize == 0) return;

    _size = setup.memManSize;
    _data = (byte_t *)malloc(_size);
    #if DEBUG
    memset(_data, 0, _size);
    #endif // DEBUG
    _head = new (_data) BlockInfo();
    _head->_dataSize = _size - BlockInfoSize;
    _tail = _head;
    _firstFree = _head;

    #if DEBUG
    // printl("MEM MAN RANGE %*p—%*p", 8, _data, 8, _data+_size);
    // printl("MEM MAN DATA SIZE %zu", _size);
    // printFreeSizes();
    // printl("BlockInfoSize %zu", BlockInfoSize);
    // printl("Request size %zu", sizeof(Request));
    #endif // DEBUG

    // create some special blocks on init
    createRequestResult();
    assert(_request && _result && "Did not create request/result block.");
    _fsa = createFSA(setup.memManFSA);
    assert(_fsa && "Failed to create fsa.");
    if (_fsa) {
        // _fsa->test();
        _fsaBlock = blockForPtr(_fsa);
        assert(_fsaBlock && "FSA block error.");
    }
    if (setup.memManFrameStackSize) {
        FrameStack * fs = createFrameStack(setup.memManFrameStackSize);
        if (fs && frameStack) {
            *frameStack = fs;
        }
    }

}

void MemMan::startFrame(size_t frame) {
    #if DEBUG
    if (!_data) return;
    _frame = frame;
    #endif // DEBUG
}

void MemMan::endFrame() {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG

    mergeAllAdjacentFreeBlocks();
    // updateFirstFree();

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG
}

void MemMan::shutdown() {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG

    free(_data);
    _data = nullptr;
    _size = 0;
    _head = nullptr;
    _tail = nullptr;
    _firstFree = nullptr;
}

byte_t const * MemMan::data() const {
    return _data;
}

size_t MemMan::size() const {
    return _size;
}

MemMan::BlockInfo * MemMan::firstBlock() const {
    return _head;
}

MemMan::BlockInfo * MemMan::nextBlock(BlockInfo const * block) const {
    return block->_next;
}

size_t MemMan::blockCountForDisplayOnly() const {
    return _blockCount;
}

void * MemMan::request(Request const & newRequest) {
    guard_t guard{_mainMutex};
    *_request = newRequest;
    request();
    return _result->ptr;
}

/*
request() handles two main questions:
    • is the user requesting an alloc, realloc, or free?
    • can the request use the FSA (fixed-sized allocator), or a standard block?
*/
void MemMan::request() {
    // thread guard
    guard_t guard{_mainMutex};

    // clear result
    *_result = {};

    // alloc/realloc
    if (_request->size) {
        // alloc
        if (_request->ptr == nullptr) {

            // fsa
            if (
                _request->align == 0 &&
                _fsa &&
                (_result->ptr = _fsa->alloc(_request->size))
            ) {
                _result->size = _request->size;
                _result->block = _fsaBlock;
                return;
            }

            // block
            else {
                _result->block = createBlock();
                if (_result->block) {
                    if (_request->type) {
                        _result->block->_type = _request->type;
                    }
                    _result->size = _result->block->_dataSize;
                    assert(_result->size >= _request->size &&
                        "Resulting block _dataSize not big enough.");
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                }
                return;
            }
        }
        // realloc
        else {
            // ptr is in FSA?
            bool ptrInFSA = false;
            BlockInfo * block = nullptr;
            uint16_t fsaGroupIndex;
            uint16_t fsaSubBlockIndex;
            size_t fsaCurrentSize = 0;
            if (_fsa && _fsa->indicesForPtr(_request->ptr, &fsaGroupIndex, &fsaSubBlockIndex)) {
                // leave ptr where it is. current FSA group is big enough for requested size.
                fsaCurrentSize = FSA::SubBlockByteSize(fsaGroupIndex);
                if (fsaCurrentSize >= _request->size) {
                    // TODO: consider FSA align
                    _result->ptr = _request->ptr;
                    _result->size = _request->size;
                    _result->block = _fsaBlock;
                    return;
                }
                ptrInFSA = true;
            }
            // ptr is in block?
            else {
                block = blockForPtr(_request->ptr);
            }
            // TODO: reconsider this assert. Could an external ptr not reallocate into MemMan?
            assert((block || ptrInFSA) && "Invalid ptr requesting reallocation.");

            // to FSA
            void * ptr = nullptr;
            if (_fsa && (ptr = _fsa->alloc(_request->size))) {
                // fsa to fsa
                if (ptrInFSA) {
                    memcpy(ptr, _request->ptr, fsaCurrentSize);
                    _result->ptr = ptr;
                    _result->size = _request->size;
                    _result->block = _fsaBlock;
                    return;
                }
                // block to fsa
                else {
                    size_t smallerSize = (block->_dataSize < _request->size) ?
                        block->_dataSize :
                        _request->size;
                    memcpy(ptr, block->data(), smallerSize);
                    _result->ptr = ptr;
                    _result->size = _request->size;
                    _result->block = _fsaBlock;
                    return;
                }
            }

            // to block
            else {

                // fsa to block
                if (ptrInFSA) {
                    _result->block = createBlock();

                    assert(_result->block && "Could not reallocate memory.");

                    if (_request->type) {
                        _result->block->_type = _request->type;
                    }

                    memcpy(_result->block->data(), _request->ptr, fsaCurrentSize);
                    _result->size = _result->block->_dataSize;
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                    return;
                }
                // block to block
                else {
                    _result->block = resizeBlock(block);

                    assert(_result->block && "Could not reallocate memory.");

                    if (_request->type) {
                        _result->block->_type = _request->type;
                    }
                    _result->size = _result->block->_dataSize;
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                    return;
                }
            }
        }
    }
    // free
    else if (_request->ptr) {
        if (_fsa->destroy(_request->ptr)) {
            // ptr was in fsa
            #if DEBUG
            validateAllBlocks();
            #endif // DEBUG
        }
        else {
            BlockInfo * block = blockForPtr(_request->ptr);
            assert(block && "Could not free ptr, not in expected range.");
            releaseBlock(block);
        }
    }
    // do nothing (size=0,ptr=0)
}

MemMan::BlockInfo * MemMan::createBlock() {
    guard_t guard{_mainMutex};
    assert(_request && "Request object not set.");

    BlockInfo * found = nullptr;

    if (_request->high) {
        // start from tail
        for (BlockInfo * bi = _tail; bi; bi = bi->_prev) {
            if (bi->_type == MEM_BLOCK_FREE && bi->_dataSize >= _request->size) {
                found = claimBlockBack(bi);
                if (found) break;
            }
        }
    }
    else {
        // start from head
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->_dataSize >= _request->size) {
                found = claimBlock(bi);
                if (found) break;
            }
        }
    }

    if (found == nullptr) {
        fprintf(stderr,
            "No block found to meet request of size %zu, align %zu\n",
            _request->size, _request->align);
    }

    return found;
}

MemMan::BlockInfo * MemMan::createBlock(MemMan::Request const & request) {
    guard_t guard{_mainMutex};
    assert(_request && "Request object not set.");

    *_request = request;
    _result->size = _request->size;
    _result->align = _request->align;
    _result->block = createBlock();
    _result->ptr = _result->block->data();

    return _result->block;
}

void MemMan::releaseBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // get some block info
    size_t blockSize = block->blockSize();

    // move BlockInfo back to base if padding was set
    if (block->_padding > 0) {
        // get block base
        byte_t * base = block->basePtr();

        // write BlockInfo to new location
        BlockInfo bi = *block; // copy out
        memcpy(base, &bi, BlockInfoSize); // copy to new ptr

        // update block info
        block = (BlockInfo *)base;
        block->_padding = 0;
        if (block->_next) block->_next->_prev = block;
        if (block->_prev) block->_prev->_next = block;
    }

    // update block info
    block->_dataSize = blockSize - BlockInfoSize;
    block->_type = MEM_BLOCK_FREE;

    // set data bytes to 0
    #if DEBUG
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    // check to see if this newly released block changes _firstFree
    // (it may or may not, findFirstFree will check)
    findFirstFreeBlock(block);
}

FrameStack * MemMan::createFrameStack(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(FrameStack) + size});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_FRAMESTACK;
    return new (block->data()) FrameStack{size};
}

File * MemMan::createFileHandle(char const * path, bool loadNow, bool high) {
    guard_t guard{_mainMutex};

    // open to deternmine size, and also maybe load
    errno = 0;
    FILE * fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error loading file \"%s\": %d\n", path, errno);
        return nullptr;
    }
    long fileSize = getFileSize(fp);
    if (fileSize == -1) {
        fprintf(stderr, "Error reading seek position in file \"%s\": %d\n", path, errno);
        return nullptr;
    }

    // make size one bigger. load process will write 0x00 in the last byte
    // after file contents so contents can be printed as string in place.
    size_t size = (size_t)fileSize + 1;

    BlockInfo * block = createBlock({.size = size + sizeof(File), .high = high});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_FILE;
    File * f = new (block->data()) File{size, path};

    // load now if request. send fp to avoid opening twice
    if (loadNow) {
        bool success = f->load(fp);
        // not sure what to do here. block successfully created but error in load...
        // keep block creation or cancel the whole thing?
        // File::load should report actual error.
        if (!success) {
        }
    }

    // close fp
    fclose(fp);

    return f;
}

FreeList * MemMan::createFreeList(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(FreeList) + FreeList::DataSize(max)});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_FREELIST;
    return new (block->data()) FreeList{max};
}

CharKeys * MemMan::createCharKeys(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(CharKeys) + CharKeys::DataSize(max)});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_CHARKEYS;
    return new (block->data()) CharKeys{max};
}

Gobj * MemMan::createGobj(char const * gltfPath) {
    guard_t guard{_mainMutex};

    // load gltf into high memory
    File * gltf = createFileHandle(gltfPath, true, false);
    if (gltf == nullptr) {
        fprintf(stderr, "Error creating File block\n");
        return nullptr;
    }

    // base path of gltf
    BlockInfo * pathBlock = createBlock({.size = strlen(gltfPath)});
    char * basePath = (char *)pathBlock->data();
    getPath(gltfPath, basePath);

    // LOADER 4
    GLTFLoader4 loader4{gltf->data(), basePath};
    if (loader4.validData() == false) {
        fprintf(stderr, "Error creating Gobj block\n");
        releaseBlock(pathBlock);
        return nullptr;
    }

    // calc size
    loader4.calculateSize();

    // create block
    BlockInfo * block = createBlock({.size = loader4.counts.totalSize(), .align = Gobj::Align});
    if (block == nullptr) {
        fprintf(stderr, "Error creating Gobj block\n");
        releaseBlock(pathBlock);
        return nullptr;
    }
    block->_type = MEM_BLOCK_GOBJ;
    Gobj * gobj = new (block->data()) Gobj{loader4.counts};

    // load into gobj
    bool success = loader4.load(gobj);

    // early return if load failed
    if (!success) {
        fprintf(stderr, "Error loading GLTF data into game object.\n");
        releaseBlock(block);
        releaseBlock(pathBlock);
        return nullptr;
    }

    // check for exepected size
    #if DEBUG
    if (loader4.isGLB) {
        assert(
            gobj->buffer + alignSize(loader4.binDataSize(), Gobj::Align) ==
            block->data() + block->dataSize() &&
            "Gobj block unexpected size."
        );
    }
    #endif // DEBUG

    // free gltf file
    // request({.ptr=gltf, .size=0});
    releaseBlock(pathBlock);

    // debug output
    #if DEBUG
    printl("LOADED GOBJ:");
    gobj->print();
    #endif // DEBUG

    return gobj;



    // OLD LOADER 3

    // calculate the data size
    // GLTFLoader3 loader;
    // Gobj::Counts counts = loader.calcDataSize(glbJSON);

    // create and load into Gobj
    // Gobj * g = createGobj(counts);
    // BlockInfo * gobjBlock = blockForPtr(g);
    // if (g == nullptr) {
    //     fprintf(stderr, "Error creating Gobj block\n");
    //     return nullptr;
    // }
    // bool success = loader.load(
    //     g,
    //     gobjBlock->_dataSize,
    //     counts,
    //     glbJSON
    // );
    // if (!success) {
    //     fprintf(stderr, "Error loading GLTF data into Gobj\n");
    //     return nullptr;
    // }

    // loader.prettyStr(glbJSON);
    // printl("JSON DATA for %s:\n%.*s", gltfPath, jsonStrSize, loader.prettyStr(glbJSON));
}

void MemMan::createRequestResult() {
    guard_t guard{_mainMutex};

    // createBlock() depends on _request being set, so we set it temporarily
    Request tempRequest{.size = sizeof(Request) + sizeof(Result)};
    _request = &tempRequest;

    BlockInfo * block = createBlock();
    if (!block) return;
    block->_type = MEM_BLOCK_REQUEST;
    _request = new (block->data()) Request{};
    _result = new (block->data()+sizeof(Request)) Result{};
}

FSA * MemMan::createFSA(MemManFSASetup const & setup) {
    guard_t guard{_mainMutex};

    // check to see if there are any FSA groups
    size_t dataSize = FSA::DataSize(setup);
    if (dataSize == 0) return nullptr;

    // solution to specifically align both FSA and containing block here, as it
    // makes size calculatable beforehand. otherwise differnce between FSA-base
    // and FSA-data might be unpredictable, resulting in
    size_t blockDataSize = alignSize(sizeof(FSA), setup.align) + dataSize;
    BlockInfo * block = createBlock({.size = blockDataSize, .align = setup.align});
    if (!block) return nullptr;

    // printl("FSA dataSize %zu", dataSize);
    // printl("sizeof(FSA) %zu", sizeof(FSA));
    // printl("alignSize sizeof(FSA) %zu", alignSize(sizeof(FSA), setup.align));
    // printl("FSA blockDataSize %zu", blockDataSize);
    // printl("aligned sizeof(FSA) %p", alignPtr(block->data() + sizeof(FSA), setup.align));

    block->_type = MEM_BLOCK_FSA;
    FSA * fsa = new (block->data()) FSA{setup};

    // printl("FSA data (minus) base %zu", fsa->data() - block->data());
    return fsa;
}

MemMan::BlockInfo * MemMan::claimBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size even if realignment happens
    if (block->calcAlignDataSize(_request->align) < _request->size) return nullptr;

    // re-align if necessary
    if (_request->align > 0 && block->_padding != block->calcAlignPaddingSize(_request->align)) {
        // get some block info
        size_t blockSize = block->blockSize();
        byte_t * base = block->basePtr();
        size_t padding = block->calcAlignPaddingSize(_request->align);
        byte_t * newBlockInfoPtr = base + padding;

        // write BlockInfo to new location
        BlockInfo bi = *block; // copy out
        memcpy(newBlockInfoPtr, &bi, BlockInfoSize); // copy to new ptr

        // set padding bytes to 0 (setting as 0xfe for debugging)
        #if DEBUG
        memset(base, 0xfe, padding);
        #endif // DEBUG

        // update block info
        bool isFirstFree = (block == _firstFree);
        bool isHead = (block == _head);
        bool isTail = (block == _tail);
        block = (BlockInfo *)newBlockInfoPtr;
        block->_dataSize = blockSize - padding - BlockInfoSize;
        block->_padding = padding;
        // next and prev pointers need to point to new memory location of BlockInfo
        if (block->_next) {
            block->_next->_prev = block;
        }
        if (block->_prev) {
            block->_prev->_next = block;
        }
        if (isFirstFree) {
            _firstFree = block;
        }
        if (isHead) {
            _head = block;
        }
        if (isTail) {
            _tail = block;
        }
    }


    // create free block, leaving this block at requested size
    // (might fail but that's ok)
    shrinkBlock(block, _request->size);
    block->_type = MEM_BLOCK_CLAIMED;

    #if DEBUG
    // zero out data in newly claimed block
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    // if our newly claimed block happend to be _firstFree, initiate a search for new firstFree
    if (block == _firstFree) {
        findFirstFreeBlock(block);
    }

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG

    return block;
}

MemMan::BlockInfo * MemMan::claimBlockBack(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->_type == MEM_BLOCK_FREE && "Block not free.");
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size even if realignment happens
    if (block->calcAlignDataSize(_request->align) < _request->size) {
        return nullptr;
    }

    // from end of block data, move back size+align+sizeof(BlockInfo).
    // if align==0, this will be the new block location.
    // if needs alignment, will be forward slightly, but should still be able
    // to accomodate `size`.
    byte_t * newBlockPtr = block->data() + block->_dataSize - (_request->size + _request->align + sizeof(BlockInfo));
    // get padding
    size_t newBlockPadding = alignPadding((size_t)newBlockPtr, _request->align);
    // actual block location
    byte_t * newBlockAlignedPtr = newBlockPtr + newBlockPadding;

    // check potential newBlock location
    assert((size_t)newBlockAlignedPtr > (size_t)block &&
        "Unexpected location for new block pointer.");
    assert((size_t)newBlockAlignedPtr < (size_t)_data + _size &&
        "New block pointer must be in memory space.");

    // set new block info
    BlockInfo * newBlock = new (newBlockAlignedPtr) BlockInfo{};
    newBlock->_padding = newBlockPadding;
    newBlock->_prev = block;
    newBlock->_next = block->_next;
    newBlock->_type = MEM_BLOCK_CLAIMED;
    newBlock->_dataSize = (block->data() + block->_dataSize) - newBlock->data();

    #if DEBUG
    assert(newBlock->_dataSize >= _request->size && "New block's data size is not big enough.");
    #endif // DEBUG

    // update old block info
    block->_dataSize = newBlock->basePtr() - block->data();
    if (block->_next) {
        block->_next->_prev = newBlock;
    }
    block->_next = newBlock;
    if (block == _tail) {
        _tail = newBlock;
    }

    // set data bytes to 0
    #if DEBUG
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG

    return newBlock;
}

MemMan::BlockInfo * MemMan::mergeWithNextBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // next block MUST be free
    if (block->_type != MEM_BLOCK_FREE ||
        block->_next == nullptr ||
        block->_next->_type != MEM_BLOCK_FREE) {
        return nullptr;
    }

    // next is tail, so now block should be tail
    if (block->_next == _tail) {
        _tail = block;
    }
    block->_dataSize += block->_next->blockSize();
    block->_next = block->_next->_next;
    if (block->_next) {
        block->_next->_prev = block;
    }

    return block;
}

MemMan::BlockInfo * MemMan::realignBlock(BlockInfo * block, size_t relevantDataSize, size_t align) {
    // guard_t guard{_mainMutex};

    // printl("MemMan::realign");
    // printFreeSizes();

    // #if DEBUG
    // assert(block->isValid() && "Block not valid.");
    // #endif // DEBUG

    // size_t padding = block->calcAlignPaddingSize(align);

    // // already aligned!
    // if (block->_padding == padding) return nullptr;

    // // make sure we have enough room for relevant data
    // if (block->blockSize() - padding - BlockInfoSize < relevantDataSize) {
    //     return nullptr;
    // }

    // // find free space to copy data to
    // BlockInfo * tempBlock = createBlock(relevantDataSize, 0, block);
    // assert(tempBlock == nullptr && "Couldn't find temp free space to realign.");

    // // copy data to temp
    // memcpy(tempBlock->data(), block->data(), relevantDataSize);

    // // free and re-claim current block with alignment
    // block = releaseBlock(block);
    // block = claim(block, relevantDataSize, align);

    // assert(block && "Claim in realign failed.");

    // // copy relevant
    // memcpy(block->data(), tempBlock->data(), relevantDataSize);
    // releaseBlock(tempBlock);

    // return block;

    return nullptr;
}

MemMan::BlockInfo * MemMan::shrinkBlock(BlockInfo * block, size_t smallerSize) {
    guard_t guard{_mainMutex};

    // do nothing if there's not enough room to create a new free block
    if (block->_dataSize < BlockInfoSize + smallerSize) {
        return nullptr;
    }

    byte_t * newBlockLoc = block->data() + smallerSize;
    BlockInfo * newBlock = new (newBlockLoc) BlockInfo();
    newBlock->_dataSize = block->_dataSize - smallerSize - BlockInfoSize;
    block->_dataSize = smallerSize;
    newBlock->_next = block->_next;
    newBlock->_prev = block;
    block->_next = newBlock;
    if (block == _tail) {
        _tail = newBlock;
    }

    findFirstFreeBlock(newBlock);

    return block;
}

MemMan::BlockInfo * MemMan::growBlock(BlockInfo * block, size_t biggerSize, size_t align) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // is next free?
    if (block->_next && block->_next->_type == MEM_BLOCK_FREE) {
        bool isAligned = block->isAligned(align);
        size_t sizeAvailable = (isAligned) ?
            // is aligned already
            (block->_dataSize + block->_next->blockSize()) :
            // calc size after alignment
            ((block->blockSize() - BlockInfoSize - block->calcAlignPaddingSize(align)) +
                block->_next->blockSize());
        // next is free and is big enough
        if (sizeAvailable >= biggerSize) {
            size_t relevantDataSize = block->_dataSize;
            // consume next block
            mergeWithNextBlock(block);
            // move everything around to new alignment first
            if (isAligned == false) {
                assert(false && "Does this ever happen? Untested. if so, realign needs rewrite.");
                block = realignBlock(block, relevantDataSize, align);
            }
            assert(block && "Realignment failed.");
            // try to create a free block with any unneeded space
            shrinkBlock(block, relevantDataSize);
            return block;
        }
    }

    // next not free or not big enough
    // move the block to new location
    BlockInfo * newBlock = createBlock({.size = biggerSize, .align = align});
    copy(newBlock->data(), block->data());
    releaseBlock(block);
    assert(newBlock && "Not enough room to move block during grow.");

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG

    return newBlock;
}

MemMan::BlockInfo * MemMan::resizeBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};
    if (block->_dataSize > _request->size) return shrinkBlock(block, _request->size);
    if (block->_dataSize < _request->size) return growBlock(block, _request->size, _request->align);
    return block;
}

void MemMan::copy(void * dst, void * src) {
    assert(dst && src && "Both pointers must be valid.");

    enum Loc { EXTERNAL, FSA, BLOCK };
    Loc dstLoc = EXTERNAL;
    Loc srcLoc = EXTERNAL;
    BlockInfo * dstBlock = nullptr;
    BlockInfo * srcBlock = nullptr;

    // where is destination?
    if (_fsa->containsPtr(dst)) {
        dstLoc = FSA;
    }
    else if ((dstBlock = blockForPtr(dst))) {
        dstLoc = BLOCK;
    }
    // where is source?
    if (_fsa->containsPtr(src)) {
        srcLoc = FSA;
    }
    else if ((srcBlock = blockForPtr(src))) {
        srcLoc = BLOCK;
    }

    // eliminate some possibilities
    assert((dstLoc == EXTERNAL && srcLoc == EXTERNAL) == false && "Both pointers can't be external.");

    // 8 possibilities to find the size
    size_t size = 0;
    // handle block to block special
    if (dstLoc == BLOCK && srcLoc == BLOCK) {
        assert(dstBlock->_dataSize >= srcBlock->_dataSize && "Destination block not big enough.");

        dstBlock->_type = srcBlock->_type;
        memcpy(dstBlock->data(), srcBlock->data(), srcBlock->_dataSize);

        return;
    }
    // everything else just needs a size for simple memcpy
    else if (dstLoc == EXTERNAL && srcLoc == FSA     ) { size = _fsa->sizeForPtr(src); }
    else if (dstLoc == EXTERNAL && srcLoc == BLOCK   ) { size = srcBlock->dataSize(); }
    else if (dstLoc == FSA      && srcLoc == EXTERNAL) { size = _fsa->sizeForPtr(dst); }
    else if (dstLoc == FSA      && srcLoc == FSA     ) { size = _fsa->sizeForPtr(src); }
    else if (dstLoc == FSA      && srcLoc == BLOCK   ) { size = srcBlock->dataSize(); }
    else if (dstLoc == BLOCK    && srcLoc == EXTERNAL) { size = dstBlock->dataSize(); }
    else if (dstLoc == BLOCK    && srcLoc == FSA     ) { size = _fsa->sizeForPtr(src); }
    memcpy(dst, src, size);
}

void MemMan::mergeAllAdjacentFreeBlocks() {
    guard_t guard{_mainMutex};

    _blockCount = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        BlockInfo * newBi = mergeWithNextBlock(bi);
        if (newBi) bi = newBi;
        ++_blockCount;
    }
}

void MemMan::findFirstFreeBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    // if _firstFree is already earlier in list than suggested search starting point (block)
    // then abort, no search needed.
    if ((size_t)_firstFree < (size_t)block) {
        // #if DEBUG
        // printl("MemMan::findFirstFree first free (%p) already precedes given block (%p).",
        //     _firstFree, block);
        // printFreeSizes();
        // #endif // DEBUG
        return;
    }

    for (BlockInfo * bi = block; bi; bi = bi->_next) {
        if (bi->_type == MEM_BLOCK_FREE) {
            _firstFree = bi;

            #if DEBUG
            validateAllBlocks();
            #endif // DEBUG

            return;
        }
    }

    // should only happen if no free blocks exist
    _firstFree = nullptr;

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG
}

MemMan::BlockInfo * MemMan::blockForPtr(void * ptr) {
    guard_t guard{_mainMutex};
    assert(_data && _size && "MemMan not initialized.");

    if (!containsPtr(ptr)) {
        return nullptr;
    }

    BlockInfo * block = (BlockInfo *)((byte_t *)ptr - BlockInfoSize);

    if (block->isValid() == false) {
        return nullptr;
    }

    return block;
}

bool MemMan::containsPtr(void * ptr) const {
    assert(_data && _size);
    byte_t * bptr = (byte_t *)ptr;
    return (
        bptr - BlockInfoSize >= _data &&
        bptr < _data + _size
    );
}

#if DEBUG
void MemMan::validateAllBlocks() {
    guard_t guard{_mainMutex};

    assert(!_firstFree || _firstFree->isValid() && "_firstFree invalid.");
    assert(_head && _head->isValid() && "_head invalid.");
    assert(_tail && _tail->isValid() && "_tail invalid.");

    size_t i = 0;
    BlockInfo * checkFirstFree = nullptr;
    size_t totalMemManSize = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        bi->_debug_index = i;
        assert(bi->isValid() && "Block invalid.");

        // check first free is correct
        if (bi->_type == MEM_BLOCK_FREE && checkFirstFree == nullptr) {
            checkFirstFree = bi;
            if (checkFirstFree != _firstFree) {
                fprintf(stderr, "found free at %p but _firstFree is set to %p\n", bi, _firstFree);
                assert(false && "_firstFree not correct");
            }
        }

        if (i == 0) {
            assert(bi == _head && "_head not set to first in list.");
        }
        else if (bi->_next == nullptr) {
            assert(bi == _tail && "_tail not set to last in list.");
        }

        totalMemManSize += bi->blockSize();
        ++i;
    }

    // make sure that sum of all blocks' sizes is expected total size.
    assert(totalMemManSize == _size && "Total of block sizes doesn't match MemMan _size.");

    // if checkFirstFree is nullptr, then no free blocks were found.
    // in that case, check that _firstFree is also nullptr
    if (checkFirstFree == nullptr) {
        assert(_firstFree == nullptr && "No free blocks were found, but _firstFree has a value.");
    }
}
#endif // DEBUG

#if DEBUG
void MemMan::printAllBlocks() const {
    printl("printAll, this: %p", this);
    size_t totalBlocks = 0;
    size_t totalDataSize = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        bi->print(totalBlocks);
        ++totalBlocks;
    }
}
#endif // DEBUG

#if DEBUG
void MemMan::printFreeBlockSizes() const {
    printl("printFreeSizes, this: %p, (first free expected: %p)", this, _firstFree);
    size_t freeBlocks = 0;
    size_t totalBlocks = 0;
    size_t totalDataSize = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        if (bi->_type == MEM_BLOCK_FREE) {
            printl("FREE BLOCK at      %3zu,   dataSize: %8zu (%p)", totalBlocks, bi->dataSize(), bi);
            totalDataSize += bi->dataSize();
            ++freeBlocks;
        }
        ++totalBlocks;
    }
    printl("Total dataSize (in %3zu free blocks) %8zu", freeBlocks, totalDataSize);
}
#endif // DEBUG

#if DEBUG
void MemMan::printRequestResult() const {
    printl("REQUEST");
    printl("    size: %zu", _request->size);
    printl("    align: %zu", _request->align);
    printl("    ptr: %p", _request->ptr);
    printl("    type: %s", memBlockTypeStr(_request->type));
    printl("RESULT");
    printl("    size: %zu", _result->size);
    printl("    align: %zu", _result->align);
    printl("    ptr: %p", _result->ptr);
    printl("    block: %p %s", _result->block, (_result->block == _fsaBlock)?"(FSA)":"");
}
#endif // DEBUG

// void MemMan::updateFirstFree() {
// }

void * BXAllocator::realloc(void * ptr, size_t size, size_t align, char const * file, uint32_t line) {
    #if DEBUG
    assert(memMan && "Memory manager pointer not set in BXAllocator.");
    if constexpr (ShowMemManBGFXDbg) printl(
        "(%05zu) BGFX ALLOC: "
        "%011p, "
        "%*zu, "
        "%*zu, "
        "%s:%d ",
        memMan->_frame,
        ptr,
        10, size,
        2, align,
        file,
        line
    );
    #endif

    // do nothing
    if (ptr == nullptr && size == 0) {
        #if DEBUG
        if constexpr (ShowMemManBGFXDbg) printl("     RETURNS EARLY: %011p)", nullptr);
        return nullptr;
        #endif // DEBUG
    }

    MemMan::guard_t guard{memMan->_mainMutex};

    // #if DEBUG
    // if (size == 32160) {
    //     debugBreak();
    // }
    // #endif // DEBUG

    // void * ret = memManRealloc(ptr, size, (void *)memMan, align);
    memMan->request({
        .ptr = ptr,
        .size = size,
        .align = align,
        .type = MEM_BLOCK_BGFX
    });

    MemMan::Result * res = memMan->_result;

    // memMan->printRequestResult();

    #if DEBUG
    memMan->validateAllBlocks();
    if (res->ptr == nullptr && size > 0) {
        debugBreak();
    }
    if (res->size < size) {
        debugBreak();
    }
    if constexpr (ShowMemManBGFXDbg) printl("          (RETURNS: %011p)", res->ptr);
    if constexpr (ShowMemManBGFXDbg) memMan->printAllBlocks();
    if constexpr (ShowMemManBGFXDbg) memMan->printRequestResult();
    #endif // DEBUG
    return res->ptr;
}
