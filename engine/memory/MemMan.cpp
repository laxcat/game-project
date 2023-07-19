#include "MemMan.h"
#include <new>
#include <string.h>
#include <assert.h>
#include "../dev/print.h"
#include "Pool.h"
#include "Stack.h"
#include "File.h"
#include "GObj.h"
#include "FSA.h"

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

#if DEBUG
bool MemMan::BlockInfo::isValid() const {
    return (
        // magic string valid
        (memcmp(&BlockMagicString, _debug_magic, 4) == 0) &&

        // if free, padding should be 0
        (_type != MEM_BLOCK_FREE || (_type == MEM_BLOCK_FREE && _padding == 0)) &&

        // suspiciously large data size
        (_dataSize < 0xffffffff) &&

        // unlikely pointers (indicates debug-only data overflow)
        ((size_t)_next != 0xfefefefefefefefe && (size_t)_prev != 0xfefefefefefefefe) &&
        ((size_t)_next != 0xefefefefefefefef && (size_t)_prev != 0xefefefefefefefef) &&
        ((size_t)_next != 0xffffffffffffffff && (size_t)_prev != 0xffffffffffffffff) &&

        // unlikely padding
        (_padding <= 256) &&

        true
    );
}
#endif // DEBUG

#if DEBUG
void MemMan::BlockInfo::print(size_t index) const {
    #if DEBUG
    if (index == SIZE_MAX) {
        index = _debug_index;
    }
    #endif // DEBUG
    printl("BLOCK %zu %s (%p, data: %p)", index, memBlockTypeStr(_type), this, data());
    printl("    data size: %zu", _dataSize);
    printl("    padding: %zu", _padding);
    printl("    prev: %p", _prev);
    printl("    next: %p", _next);
    printl("    debug index: %zu", _debug_index);
    printl("    magic string: %.*s", 8, _debug_magic);
}
#endif // DEBUG

void MemMan::init(EngineSetup const & setup, Stack ** frameStack) {
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
    printl("MEM MAN RANGE %*p—%*p", 8, _data, 8, _data+_size);
    printl("MEM MAN DATA SIZE %zu", _size);
    printFreeSizes();
    printl("BlockInfoSize %zu", BlockInfoSize);
    printl("Request size %zu", sizeof(Request));
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
        Stack * fs = createStack(setup.memManFrameStackSize);
        if (fs && frameStack) {
            *frameStack = fs;
        }
    }

}

void MemMan::startFrame(size_t frame) {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG
    _frame = frame;
}

void MemMan::endFrame() {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG

    mergeAllAdjacentFree();
    // updateFirstFree();

    #if DEBUG
    validateAll();
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

// // generic alloc request, which can return Block or pointer within FSA
// void * MemMan::alloc(size_t size, size_t align, BlockInfo ** resultBlock) {
//     guard_t guard{_mainMutex};

//     // CHECK IF FSA WILL WORK
//     // TODO: figure out align here
//     void * subPtr;
//     if (align == 0 && _fsa && (subPtr = _fsa->alloc(size))) {
//         if (resultBlock) {
//             *resultBlock = blockForPtr(_fsa);
//         }
//         return subPtr;
//     }

//     BlockInfo * block = create(size, align);
//     if (resultBlock) {
//         *resultBlock = block;
//     }
//     return (block) ? block->data() : nullptr;
// }

void * MemMan::request(Request const & newRequest) {
    guard_t guard{_mainMutex};
    *_request = newRequest;
    request();
    return _result->ptr;
}

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
                _result->block = create(
                    _request->size,
                    _request->align,
                    _request->copyFrom
                );
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
                    _result->block = create(_request->size, _request->align);

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
                    _result->block = resize(block, _request->size, _request->align);

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
            validateAll();
            #endif // DEBUG
        }
        else {
            BlockInfo * block = blockForPtr(_request->ptr);
            assert(block && "Could not free ptr, not in expected range.");
            release(block);
        }
    }
    // do nothing (size=0,ptr=0)
}

MemMan::BlockInfo * MemMan::create(size_t size, size_t align, BlockInfo * copyFrom) {
    guard_t guard{_mainMutex};

    BlockInfo * found = nullptr;

    if (align) {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->calcAlignDataSize(align) >= size) {
                found = claim(bi, size, align, copyFrom);
                if (found) break;
            }
        }
    }
    else {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->_dataSize >= size) {
                found = claim(bi, size, 0, copyFrom);
                if (found) break;
            }
        }
    }

    if (found == nullptr) {
        fprintf(stderr, "No block found to meet request of size %zu, align %zu\n", size, align);
    }

    return found;
}

void MemMan::release(BlockInfo * block) {
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
    findFirstFree(block);
}

// bool MemMan::destroy(void * ptr) {
//     guard_t guard{_mainMutex};

//     // CHECK IF ptr WAS WITHIN FSA
//     if (_fsa && _fsa->destroy(ptr)) {
//         return true;
//     }

//     BlockInfo * block = blockForPtr(ptr);
//     return (block && release(block)) ? true : false;
// }

Stack * MemMan::createStack(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = create(sizeof(Stack) + size);
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_STACK;
    return new (block->data()) Stack{size};
}

void MemMan::createRequestResult() {
    guard_t guard{_mainMutex};

    BlockInfo * block = create(sizeof(Request) + sizeof(Result));
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
    BlockInfo * block = create(blockDataSize, setup.align);
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

MemMan::BlockInfo * MemMan::claim(BlockInfo * block, size_t size, size_t align, BlockInfo * copyFrom) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size even if realignment happens
    if (block->calcAlignDataSize(align) < size) return nullptr;

    // re-align if necessary
    if (align > 0 && block->_padding != block->calcAlignPaddingSize(align)) {
        // get some block info
        size_t blockSize = block->blockSize();
        byte_t * base = block->basePtr();
        size_t padding = block->calcAlignPaddingSize(align);
        byte_t * newBlockInfoPtr = base + padding;

        // write BlockInfo to new location
        BlockInfo bi = *block; // copy out
        memcpy(newBlockInfoPtr, &bi, BlockInfoSize); // copy to new ptr

        // set padding bytes to 0
        #if DEBUG
        memset(base, 0xfe, padding);
        #endif // DEBUG

        // update block info
        bool isFirstFree = (block == _firstFree);
        block = (BlockInfo *)newBlockInfoPtr;
        block->_dataSize = blockSize - padding - BlockInfoSize;
        block->_padding = padding;
        // next and prev pointers need to point to new memory location of BlockInfo
        if (block->_next) block->_next->_prev = block;
        if (block->_prev) block->_prev->_next = block;
        if (isFirstFree) _firstFree = block;
    }


    // create free block, leaving this block at requested size
    // (might fail but that's ok)
    shrink(block, size);
    block->_type = MEM_BLOCK_CLAIMED;

    // if we are copying data from another block, copy then release other block,
    // then find first free starting with our newly released copyFrom block
    if (copyFrom && copyFrom->_dataSize <= size) {
        assert(block->_dataSize >= copyFrom->_dataSize && "Newly claimed block not big enough for copyFrom block data.");

        block->_type = copyFrom->_type;
        memcpy(block->data(), copyFrom->data(), copyFrom->_dataSize);
        release(copyFrom);

        #if DEBUG
        validateAll();
        #endif // DEBUG

    }
    // no copyFrom block. zero out data in newly claimed block.
    // if our newly claimed block happend to be _firstFree, initiate a search for new firstFree
    else {
        // set data bytes to 0
        #if DEBUG
        memset(block->data(), 0, block->_dataSize);
        #endif // DEBUG
        if (block == _firstFree) {
            findFirstFree(block);
        }

        #if DEBUG
        validateAll();
        #endif // DEBUG
    }

    return block;

}

MemMan::BlockInfo * MemMan::mergeWithNext(BlockInfo * block) {
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

    block->_dataSize += block->_next->blockSize();
    block->_next = block->_next->_next;
    if (block->_next) block->_next->_prev = block;

    return block;
}

MemMan::BlockInfo * MemMan::realign(BlockInfo * block, size_t relevantDataSize, size_t align) {
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
    // BlockInfo * tempBlock = create(relevantDataSize, 0, block);
    // assert(tempBlock == nullptr && "Couldn't find temp free space to realign.");

    // // copy data to temp
    // memcpy(tempBlock->data(), block->data(), relevantDataSize);

    // // free and re-claim current block with alignment
    // block = release(block);
    // block = claim(block, relevantDataSize, align);

    // assert(block && "Claim in realign failed.");

    // // copy relevant
    // memcpy(block->data(), tempBlock->data(), relevantDataSize);
    // release(tempBlock);

    // return block;

    return nullptr;
}

MemMan::BlockInfo * MemMan::shrink(BlockInfo * block, size_t smallerSize) {
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
    if (block == _tail) _tail = newBlock;

    findFirstFree(newBlock);

    return block;
}

MemMan::BlockInfo * MemMan::grow(BlockInfo * block, size_t biggerSize, size_t align) {
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
            mergeWithNext(block);
            // move everything around to new alignment first
            if (isAligned == false) {
                assert(false && "Does this ever happen? Untested. if so, realign needs rewrite.");
                block = realign(block, relevantDataSize, align);
            }
            assert(block && "Realignment failed.");
            // try to create a free block with any unneeded space
            shrink(block, relevantDataSize);
            return block;
        }
    }

    // next not free or not big enough
    // move the block to new location
    BlockInfo * newBlock = create(biggerSize, align, block);
    assert(newBlock && "Not enough room to move block during grow.");

    #if DEBUG
    validateAll();
    #endif // DEBUG

    return newBlock;
}

MemMan::BlockInfo * MemMan::resize(BlockInfo * block, size_t newSize, size_t align) {
    guard_t guard{_mainMutex};
    if (block->_dataSize > newSize) return shrink(block, newSize);
    if (block->_dataSize < newSize) return grow(block, newSize, align);
    return block;
}

void MemMan::mergeAllAdjacentFree() {
    guard_t guard{_mainMutex};

    _blockCount = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        BlockInfo * newBi = mergeWithNext(bi);
        if (newBi) bi = newBi;
        ++_blockCount;
    }
}

void MemMan::findFirstFree(BlockInfo * block) {
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
            validateAll();
            #endif // DEBUG

            return;
        }
    }

    // should only happen if no free blocks exist
    _firstFree = nullptr;

    #if DEBUG
    validateAll();
    #endif // DEBUG
}

MemMan::BlockInfo * MemMan::blockForPtr(void * ptr) {
    guard_t guard{_mainMutex};

    byte_t * bptr = (byte_t *)ptr;
    assert(
        _data && _size &&
        bptr - BlockInfoSize >= _data &&
        bptr < _data + _size &&
        "Invalid ptr range."
    );
    BlockInfo * block = (BlockInfo *)(bptr - BlockInfoSize);
    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG
    return block;
}

#if DEBUG
void MemMan::validateAll() {
    guard_t guard{_mainMutex};

    assert(!_firstFree || _firstFree->isValid() && "_firstFree invalid.");

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
void MemMan::printAll() const {
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
void MemMan::printFreeSizes() const {
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
    printl("    copyFrom: %p", _request->copyFrom);
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
    memMan->validateAll();
    if (res->ptr == nullptr && size > 0) {
        debugBreak();
    }
    if (res->size < size) {
        debugBreak();
    }
    if constexpr (ShowMemManBGFXDbg) printl("          (RETURNS: %011p)", res->ptr);
    if constexpr (ShowMemManBGFXDbg) memMan->printAll();
    if constexpr (ShowMemManBGFXDbg) memMan->printRequestResult();
    #endif // DEBUG
    return res->ptr;
}
