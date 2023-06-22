#include "MemMan2.h"
#include <new>
#include <string.h>
#include <assert.h>
#include "../dev/print.h"
#include "Pool.h"
#include "Stack.h"
#include "File.h"
#include "GObj.h"
#include "FSA.h"

constexpr static bool ShowMemManBGFXDbg = false;

size_t MemMan2::BlockInfo::paddingSize() const {
    return _padding;
}

size_t MemMan2::BlockInfo::dataSize() const {
    return _dataSize;
}

size_t MemMan2::BlockInfo::blockSize() const {
    return _padding + BlockInfoSize + _dataSize;
}

MemBlockType MemMan2::BlockInfo::type() const {
    return _type;
}

byte_t const * MemMan2::BlockInfo::data() const {
    return (byte_t *)this + BlockInfoSize;
}
byte_t * MemMan2::BlockInfo::data() {
    return (byte_t *)this + BlockInfoSize;
}

byte_t const * MemMan2::BlockInfo::calcUnalignedDataLoc() const {
    return data() - _padding;
}

size_t MemMan2::BlockInfo::calcAlignPaddingSize(size_t align) const {
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

size_t MemMan2::BlockInfo::calcAlignDataSize(size_t align) const {
    // there may be padding already set, but this should return what the value
    // would be if re-aligned to parameter align
    return (_padding + _dataSize) - calcAlignPaddingSize(align);
}

bool MemMan2::BlockInfo::isAligned(size_t align) const {
    return (_padding == calcAlignPaddingSize(align));
}

byte_t const * MemMan2::BlockInfo::basePtr() const {
    return data() - BlockInfoSize - _padding;
}
byte_t * MemMan2::BlockInfo::basePtr() {
    return data() - BlockInfoSize - _padding;
}

#if DEBUG
bool MemMan2::BlockInfo::isValid() const {
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
void MemMan2::BlockInfo::print(size_t index) const {
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

void MemMan2::init(EngineSetup const & setup, Stack ** frameStack) {
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

    printl("MEM MAN RANGE %*p—%*p", 8, _data, 8, _data+_size);
    printl("MEM MAN DATA SIZE %zu", _size);
    printFreeSizes();
    printl("BlockInfoSize %zu", BlockInfoSize);

    // create some special blocks on init
    _fsa = createFSA(setup.memManFSA);
    if (setup.memManFrameStackSize) {
        Stack * fs = createStack(setup.memManFrameStackSize);
        if (fs && frameStack) {
            *frameStack = fs;
        }
    }
}

void MemMan2::startFrame(size_t frame) {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG
    _frame = frame;
}

void MemMan2::endFrame() {
    #if DEBUG
    if (!_data) return;
    #endif // DEBUG

    mergeAllAdjacentFree();
    // updateFirstFree();

    #if DEBUG
    validateAll();
    #endif // DEBUG
}

void MemMan2::shutdown() {
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

byte_t const * MemMan2::data() const {
    return _data;
}

size_t MemMan2::size() const {
    return _size;
}

MemMan2::BlockInfo * MemMan2::firstBlock() const {
    return _head;
}

MemMan2::BlockInfo * MemMan2::nextBlock(BlockInfo const * block) const {
    return block->_next;
}

size_t MemMan2::blockCountForDisplayOnly() const {
    return _blockCount;
}

// generic alloc request, which can return Block or pointer within FSA
void * MemMan2::alloc(size_t size, size_t align, BlockInfo ** resultBlock) {
    guard_t guard{_mainMutex};

    // CHECK IF FSA WILL WORK
    // TODO: figure out align here
    void * subPtr;
    if (align == 0 && _fsa && (subPtr = _fsa->alloc(size))) {
        if (resultBlock) {
            *resultBlock = blockForPtr(_fsa);
        }
        return subPtr;
    }

    BlockInfo * block = request(size, align);
    if (resultBlock) {
        *resultBlock = block;
    }
    return (block) ? block->data() : nullptr;
}

MemMan2::BlockInfo * MemMan2::request(size_t size, size_t align, BlockInfo * copyFrom) {
    guard_t guard{_mainMutex};

    BlockInfo * found = nullptr;

    if (align) {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->calcAlignDataSize(align) >= size) {
                found = claim(bi, size, align, copyFrom);
                bi = found; // claim can modify block location
                break;
            }
        }
    }
    else {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->_dataSize >= size) {
                found = claim(bi, size, 0, copyFrom);
                bi = found; // claim can modify block location
                break;
            }
        }
    }

    if (found == nullptr) {
        fprintf(stderr, "No block found to meet request of size %zu, align %zu\n", size, align);
    }

    return found;
}

MemMan2::BlockInfo * MemMan2::release(BlockInfo * block) {
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

    return block;
}

bool MemMan2::destroy(void * ptr) {
    guard_t guard{_mainMutex};

    // CHECK IF ptr WAS WITHIN FSA
    if (_fsa && _fsa->destroy(ptr)) {
        return true;
    }

    BlockInfo * block = blockForPtr(ptr);
    return (block && release(block)) ? true : false;
}

Stack * MemMan2::createStack(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = request(sizeof(Stack) + size);
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_STACK;
    return new (block->data()) Stack{size};
}

FSA * MemMan2::createFSA(MemManFSASetup const & setup) {
    guard_t guard{_mainMutex};

    // check to see if there are any FSA groups
    size_t dataSize = FSA::DataSize(setup);
    if (dataSize == 0) return nullptr;

    BlockInfo * block = request(sizeof(FSA) + dataSize);
    if (!block) return nullptr;

    block->_type = MEM_BLOCK_FSA;
    FSA * fsa = new (block->data()) FSA{setup};
    return fsa;
}

MemMan2::BlockInfo * MemMan2::claim(BlockInfo * block, size_t size, size_t align, BlockInfo * copyFrom) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size
    if (block->calcAlignDataSize(align) < size) return nullptr;

    // re-align if necessary.
    if (align > 0) {
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

MemMan2::BlockInfo * MemMan2::mergeWithNext(BlockInfo * block) {
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

MemMan2::BlockInfo * MemMan2::realign(BlockInfo * block, size_t relevantDataSize, size_t align) {
    // guard_t guard{_mainMutex};

    // printl("MemMan2::realign");
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
    // BlockInfo * tempBlock = request(relevantDataSize, 0, block);
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

MemMan2::BlockInfo * MemMan2::shrink(BlockInfo * block, size_t smallerSize) {
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

    return block;
}

MemMan2::BlockInfo * MemMan2::grow(BlockInfo * block, size_t biggerSize, size_t align) {
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
    BlockInfo * newBlock = request(biggerSize, align, block);
    assert(newBlock && "Not enough room to move block during grow.");

    #if DEBUG
    validateAll();
    #endif // DEBUG

    return newBlock;
}

MemMan2::BlockInfo * MemMan2::resize(BlockInfo * block, size_t newSize, size_t align) {
    guard_t guard{_mainMutex};
    if (block->_dataSize > newSize) return shrink(block, newSize);
    if (block->_dataSize < newSize) return grow(block, newSize, align);
    return block;
}

void MemMan2::mergeAllAdjacentFree() {
    guard_t guard{_mainMutex};

    _blockCount = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        mergeWithNext(bi);
        ++_blockCount;
    }
}

void MemMan2::findFirstFree(BlockInfo * block) {
    guard_t guard{_mainMutex};

    // if _firstFree is already earlier in list than suggested search starting point (block)
    // then abort, no search needed.
    if ((size_t)_firstFree < (size_t)block) {
        // #if DEBUG
        // printl("MemMan2::findFirstFree first free (%p) already precedes given block (%p).",
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

MemMan2::BlockInfo * MemMan2::blockForPtr(void * ptr) {
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
void MemMan2::validateAll() {
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
void MemMan2::printAll() const {
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
void MemMan2::printFreeSizes() const {
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

// void MemMan2::updateFirstFree() {
// }

void * memManAlloc(size_t size, void * userData, size_t align) {
    if (!userData) return nullptr;
    MemMan2 * memMan2 = (MemMan2 *)userData;

    MemMan2::guard_t guard{memMan2->_mainMutex};

    if (!size) return nullptr;
    MemMan2::BlockInfo * block = memMan2->request(size);
    if (!block) {
        fprintf(stderr, "Unable to alloc memory: %zu\n", size);
        assert(false);
    }
    block->_type = MEM_BLOCK_EXTERNAL;

    // printl("memManAlloc");
    // memMan2->printFreeSizes();

    return block->data();
}

void * memManRealloc(void * ptr, size_t size, void * userData, size_t align) {
    if (!userData) return nullptr;
    MemMan2 * memMan2 = (MemMan2 *)userData;

    MemMan2::guard_t guard{memMan2->_mainMutex};

    if (ptr == nullptr) return memManAlloc(size, userData, align);
    if (size == 0) {
        memManFree(ptr, userData);
        return nullptr;
    }
    MemMan2::BlockInfo * block = memMan2->blockForPtr(ptr);

    #if DEBUG
    assert(block->isValid() && "Block not valid. (memManRealloc)");
    memMan2->validateAll();
    #endif // DEBUG

    // printl("block before resize:");
    // block->print();

    // if (block->_dataSize == 2 && size == 32160) {
    //     debugBreak();
    // }

    block = memMan2->resize(block, size, align);

    if (block->_dataSize < size) {
        memMan2->validateAll();
        memMan2->printAll();
        printl("Something went wrong! Data size not big enough.");
        debugBreak();
    }

    // printl("memManRealloc %p", block);
    // memMan2->printFreeSizes();

    return (block) ? block->data() : nullptr;
}

void memManFree(void * ptr, void * userData) {
    if (!userData) return;
    MemMan2 * memMan2 = (MemMan2 *)userData;

    MemMan2::guard_t guard{memMan2->_mainMutex};

    if (!ptr) return;
    memMan2->destroy(ptr);

    // printl("memManFree");
    // memMan2->printFreeSizes();
}

void * BXAllocator::realloc(void * ptr, size_t size, size_t align, char const * file, uint32_t line) {
    #if DEBUG
        assert(memMan2 && "Memory manager pointer not set in BXAllocator.");
    #endif

    if constexpr (ShowMemManBGFXDbg) printl(
        "(%05zu) BGFX ALLOC: "
        "%011p, "
        "%*zu, "
        "%*zu, "
        "%s:%d ",
        memMan2->_frame,
        ptr,
        10, size,
        2, align,
        file,
        line
    );

    // do nothing
    if (ptr == nullptr && size == 0) {
        if constexpr (ShowMemManBGFXDbg) printl("     RETURNS EARLY: %011p)", nullptr);
        return nullptr;
    }

    MemMan2::guard_t guard{memMan2->_mainMutex};

    // #if DEBUG
    // if (size == 32160) {
    //     debugBreak();
    // }
    // #endif // DEBUG

    void * ret = memManRealloc(ptr, size, (void *)memMan2, align);

    #if DEBUG
    if (ret == nullptr && size > 0) {
        debugBreak();
    }
    #endif // DEBUG

    if (ptr == nullptr && size) {
        memMan2->blockForPtr(ret)->_type = MEM_BLOCK_BGFX;
    }

    // printl("bgfx realloc end");
    // memMan2->printFreeSizes();

    if constexpr (ShowMemManBGFXDbg) printl("          (RETURNS: %011p)", ret);
    return ret;
}
