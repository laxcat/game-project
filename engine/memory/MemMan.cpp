#include "MemMan.h"
#include <assert.h>
#if DEBUG
#include "../dev/print.h"
#endif // DEBUG
#include "mem_utils.h"
#include "FSA.h"

#if DEBUG
constexpr static bool ShowMemManBGFXDbg = false;
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
    if (setup.memManAutoReleaseBufferSize) {
        _autoReleaseBuffer = createAutoReleaseBuffer(setup.memManAutoReleaseBufferSize);
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

    autoReleaseEndFrame();
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
                addAutoRelease();
                return;
            }

            // block
            else {
                _result->block = createBlock();
                if (_result->block) {
                    _result->size = _result->block->_dataSize;
                    assert(_result->size >= _request->size &&
                        "Resulting block _dataSize not big enough.");
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                    addAutoRelease();
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
                    updateAutoRelease();
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
                    updateAutoRelease();
                    return;
                }
            }

            // to block
            else {

                // fsa to block
                if (ptrInFSA) {
                    _result->block = createBlock();

                    assert(_result->block && "Could not reallocate memory.");

                    memcpy(_result->block->data(), _request->ptr, fsaCurrentSize);
                    _result->size = _result->block->_dataSize;
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                    updateAutoRelease();
                    return;
                }
                // block to block
                else {
                    _result->block = resizeBlock(block);

                    assert(_result->block && "Could not reallocate memory.");

                    _result->size = _result->block->_dataSize;
                    _result->align = _request->align;
                    _result->ptr = _result->block->data();
                    updateAutoRelease();
                    return;
                }
            }
        }
    }
    // free
    else if (_request->ptr) {
        if (_fsa->destroy(_request->ptr)) {
            // ptr was in fsa
            removeAutoRelease();
        }
        else {
            BlockInfo * block = blockForPtr(_request->ptr);
            assert(block && "Could not free ptr, not in expected range.");
            releaseBlock(block);
            removeAutoRelease();
        }
        #if DEBUG
        validateAllBlocks();
        #endif // DEBUG
    }
    // do nothing (size=0,ptr=0)
}

void MemMan::autoReleaseEndFrame() {
    for (int i = 0; i < _autoReleaseBuffer->size(); ++i) {
        AutoRelease & alloc = _autoReleaseBuffer->data()[i];
        // release allocation when lifetime is 0
        if (alloc.lifetime == 0) {
            // setting lifetime = -1 here will tell request() to skip all
            // auto-release consideration. we're handling that manually below.
            request({.ptr = alloc.ptr, .size = 0, .lifetime = -1});
            _autoReleaseBuffer->remove(i);
            --i;
            continue;
        }
        --alloc.lifetime;
    }
}

void MemMan::addAutoRelease() {
    if (_request->lifetime < 0) {
        return;
    }
    _autoReleaseBuffer->append({
        .ptr = _result->ptr,
        .lifetime = _request->lifetime
    });
}

void MemMan::updateAutoRelease() {
    if (_request->lifetime < 0) {
        return;
    }
    for (int i = 0; i < _autoReleaseBuffer->size(); ++i) {
        auto & alloc = _autoReleaseBuffer->data()[i];
        if (alloc.ptr == _request->ptr) {
            alloc.ptr = _result->ptr;
            return;
        }
    }
}

void MemMan::removeAutoRelease() {
    if (_request->lifetime < 0) {
        return;
    }
    for (int i = 0; i < _autoReleaseBuffer->size(); ++i) {
        auto & alloc = _autoReleaseBuffer->data()[i];
        if (alloc.ptr == _request->ptr) {
            _autoReleaseBuffer->remove(i);
            return;
        }
    }
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
