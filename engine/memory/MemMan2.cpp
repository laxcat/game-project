#include "MemMan2.h"
#include <new>
#include <string.h>
#include "../dev/print.h"
#include "Pool.h"
#include "Stack.h"
#include "File.h"
#include "GObj.h"
#include "FSA.h"

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

    if (align == 0) return 0;

    // block may be aligned, but treat it as though its not to get potential
    // new align location. this is then used to calc new padding size.
    size_t dataPtrAsNum = (size_t)calcUnalignedDataLoc();
    size_t remainder = dataPtrAsNum % align;
    return (remainder) ? align - remainder : 0;
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

        true
    );
}
#endif // DEBUG

void MemMan2::init(EngineSetup const & setup) {
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
    printl("BlockInfoSize %zu", BlockInfoSize);

    // create some special blocks on init
    // if (setup.memManFrameStackSize) {
    //     createStack(setup.memManFrameStackSize);
    // }
    _fsa = createFSA(setup.memManFSA);
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

MemMan2::BlockInfo * MemMan2::request(size_t size, size_t align) {
    guard_t guard{_mainMutex};

    BlockInfo * found = nullptr;

    if (align) {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->calcAlignDataSize(align) >= size) {
                found = claim(bi, size, align);
                bi = found; // claim can modify block location
                break;
            }
        }
    }
    else {
        for (BlockInfo * bi = _firstFree; bi; bi = bi->_next) {
            if (bi->_type == MEM_BLOCK_FREE && bi->_dataSize >= size) {
                found = claim(bi, size);
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

    if ((byte_t *)block < (byte_t *)_firstFree) {
        _firstFree = block;
    }

    return block;
}

bool MemMan2::destroy(void * ptr) {
    BlockInfo * block = blockForPtr(ptr);
    return (release(block)) ? true : false;
}

FSA * MemMan2::createFSA(MemManFSASetup const & setup) {
    size_t fsaDataSize = FSA::DataSize(setup);
    if (fsaDataSize == 0) return nullptr;

    BlockInfo * block = request(fsaDataSize);
    if (!block) return nullptr;

    block->_type = MEM_BLOCK_FSA;
    FSA * fsa = new (block->data()) FSA{setup};
    return fsa;
}

MemMan2::BlockInfo * MemMan2::claim(BlockInfo * block, size_t size, size_t align) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size
    if (block->calcAlignDataSize(align) < size) return nullptr;

    // exit quickly if no alignment needed.
    if (align == 0) {
        block->_type = MEM_BLOCK_CLAIMED;
        shrink(block, size);
        findFirstFree(block);
        return block;
    }

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
    block = (BlockInfo *)newBlockInfoPtr;
    block->_dataSize = blockSize - padding - BlockInfoSize;
    block->_padding = padding;
    block->_type = MEM_BLOCK_CLAIMED;
    // next and prev pointers need to point to new memory location of BlockInfo
    if (block->_next) block->_next->_prev = block;
    if (block->_prev) block->_prev->_next = block;

    // create free block, leaving this block at requested size
    // (might fail but that's ok)
    BlockInfo * shrunken = shrink(block, size);

    // set data bytes to 0
    #if DEBUG
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    findFirstFree(block);

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
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    size_t padding = block->calcAlignPaddingSize(align);

    // already aligned!
    if (block->_padding == padding) return nullptr;

    // make sure we have enough room for relevant data
    if (block->blockSize() - padding - BlockInfoSize < relevantDataSize) {
        return nullptr;
    }

    // find free space to copy data to
    BlockInfo * tempBlock = request(relevantDataSize);
    assert(tempBlock == nullptr && "Couldn't find temp free space to realign.");

    // copy data to temp
    memcpy(tempBlock->data(), block->data(), relevantDataSize);

    // free and re-claim current block with alignment
    block = release(block);
    block = claim(block, relevantDataSize, align);

    assert(block && "Claim in realign failed.");

    // copy relevant
    memcpy(block->data(), tempBlock->data(), relevantDataSize);
    release(tempBlock);

    return block;
}

MemMan2::BlockInfo * MemMan2::shrink(BlockInfo * block, size_t smallerSize) {
    guard_t guard{_mainMutex};

    // do nothing if there's not enough room to create a new free block
    if (block->_dataSize < BlockInfoSize + smallerSize) return nullptr;

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

    bool isNextFree = (block->_next && block->_next->_type == MEM_BLOCK_FREE);
    bool isAligned = block->isAligned(align);
    bool isNextBigEnough = (isAligned) ?
        // is aligned already
        (block->_dataSize + block->_next->blockSize() <= biggerSize) :
        // needs alignment
        ((block->blockSize() - block->calcAlignPaddingSize(align) - BlockInfoSize) +
            block->_next->blockSize() <= biggerSize);

    // current location is aligned as requested. (includes align == 0)
    // we don't know if it will stay here yet.
    if (isNextBigEnough) {
        size_t relevantDataSize = block->_dataSize;
        // consume next block
        mergeWithNext(block);
        // move everything around to new alignment first
        if (isAligned == false) block = realign(block, relevantDataSize, align);
        assert(block && "Realignment failed.");
        // try to create a free block with any unneeded space
        shrink(block, relevantDataSize);
        return block;
    }


    return nullptr;
}

MemMan2::BlockInfo * MemMan2::resize(BlockInfo * block, size_t newSize, size_t align) {
    guard_t guard{_mainMutex};
    if (block->_dataSize > newSize) return shrink(block, newSize);
    if (block->_dataSize < newSize) return grow(block, newSize, align);
    return block;
}

void MemMan2::mergeAllAdjacentFree() {
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        mergeWithNext(bi);
    }
}

void MemMan2::findFirstFree(BlockInfo * block) {
    for (BlockInfo * bi = block; bi; bi = bi->_next) {
        if (bi->_type == MEM_BLOCK_FREE) {
            _firstFree = bi;
        return;
        }
    }
    // should only happen if no free blocks exist
    _firstFree = _tail;
}

MemMan2::BlockInfo * MemMan2::blockForPtr(void * ptr) {
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
    size_t i = 0;
    BlockInfo * checkFirstFree = nullptr;
    size_t totalMemManSize = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        bi->_debug_index = i;
        assert(bi->isValid() && "Block invalid.");

        // check first free is correct
        if (bi->_type == MEM_BLOCK_FREE && checkFirstFree == nullptr) {
            checkFirstFree = bi;
            assert(checkFirstFree == _firstFree && "_firstFree not correct");
        }

        totalMemManSize += bi->blockSize();
    }

    assert(totalMemManSize == _size && "Total of block sizes doesn't match MemMan _size.");
}
#endif // DEBUG

// void MemMan2::updateFirstFree() {
// }
