#include "MemMan.h"
#include "mem_utils.h"

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
    // TODO: anything that is done in both claimBlock fns should move here
    else {
        found->_type = _request->type;
    }

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG

    return found;
}

MemMan::BlockInfo * MemMan::createBlock(MemMan::Request const & request) {
    guard_t guard{_mainMutex};
    assert(_request && "Request object not set.");
    *_request = request;
    _result->block = createBlock();
    if (_result->block) {
        _result->ptr = _result->block->data();
        _result->size = _result->block->dataSize();
        _result->align = _request->align;
        addAutoRelease();
    }
    return _result->block;
}

void MemMan::releaseBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // get some block info
    size_t blockSize = block->blockSize();

    // update block info
    block->_type = MEM_BLOCK_FREE;

    // move BlockInfo back to base if padding was set
    if (block->_padding > 0) {
        block = realignBlock(block, 0);
    }

    // update block info
    block->_dataSize = blockSize - BlockInfoSize;
    _freeBlockSize += block->blockSize();

    // set data bytes to 0
    #if DEBUG
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    linkBlocks(block->_prev, block, block->_next);

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG
}

MemMan::BlockInfo * MemMan::claimBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // ensure sufficient size even if realignment happens
    if (block->calcAlignDataSize(_request->align) < _request->size) return nullptr;

    // re-align if necessary
    size_t newPadding = block->calcAlignPaddingSize(_request->align);
    if (_request->align > 0 && block->_padding != newPadding) {
        block = realignBlock(block, newPadding);
    }

    block->_type = MEM_BLOCK_CLAIMED;
    // create free block, leaving this block at requested size
    // (might fail but that's ok)
    shrinkBlock(block, _request->size);

    _freeBlockSize -= block->blockSize();

    #if DEBUG
    // zero out data in newly claimed block
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    // block may have moved in realign, so link all three
    linkBlocks(block->_prev, block, block->_next);

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

    // from end of block data, move back size+align+BlockInfoSize.
    // if align==0, this will be the new block location.
    // if needs alignment, will be forward slightly, but should still be able
    // to accomodate `size`.
    byte_t * newBlockPtr = block->data() + block->_dataSize - (_request->size + _request->align + BlockInfoSize);
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
    newBlock->_padding = (uint32_t)newBlockPadding;
    newBlock->_type = MEM_BLOCK_CLAIMED;
    newBlock->_dataSize = (block->data() + block->_dataSize) - newBlock->data();

    #if DEBUG
    assert(newBlock->_dataSize >= _request->size && "New block's data size is not big enough.");
    #endif // DEBUG

    // update old block info
    block->_dataSize = newBlock->basePtr() - block->data();
    _freeBlockSize -= newBlock->blockSize();

    // set data bytes to 0
    #if DEBUG
    memset(block->data(), 0, block->_dataSize);
    #endif // DEBUG

    // block never realigns, so block->_prev not necessary
    linkBlocks(block, newBlock, block->_next);

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

    linkBlocks(block, block->_next->_next);

    return block;
}

void MemMan::linkBlockList(BlockInfo ** blockList, uint16_t nBlocks) {
    assert(blockList && nBlocks && "Unexpected parameters.");

    BlockInfo * lastNonNull = nullptr;
    BlockInfo * firstNonNull = nullptr;
    BlockInfo * foundTail = nullptr;
    bool shouldFindFree = false;

    for (uint16_t i = 0; i < nBlocks; ++i) {
        BlockInfo * b = blockList[i];
        if (b == nullptr) {
            continue;
        }

        if (firstNonNull == nullptr) {
            firstNonNull = b;
        }
        lastNonNull = b;

        if (b == _head && i != 0) {
            _head = firstNonNull;
        }
        if (b == _tail && i != nBlocks-1) {
            foundTail = b;
        }
        if (!shouldFindFree &&
            (b->_type == MEM_BLOCK_FREE ||
            b == _firstFree
            )) {
            shouldFindFree = true;
        }

        if (i > 0) {
            b->_prev = blockList[i-1];
        }
        if (i < nBlocks-1) {
            b->_next = blockList[i+1];
        }
    }

    if (foundTail && lastNonNull) {
        _tail = lastNonNull;
    }

    if (shouldFindFree) {
        findFirstFreeBlock(firstNonNull);
    }
}

MemMan::BlockInfo * MemMan::realignBlock(BlockInfo * block, size_t padding) {
    guard_t guard{_mainMutex};

    assert(block->_type == MEM_BLOCK_FREE && "Block must be free to realign.");

    bool isFirstFree = (block == _firstFree);
    bool isHead = (block == _head);
    bool isTail = (block == _tail);
    size_t blockSize = block->blockSize();

    // get block base
    byte_t * base = block->basePtr();

    // write BlockInfo to new location
    BlockInfo bi = *block; // copy out
    memcpy(base + padding, &bi, BlockInfoSize); // copy to new ptr

    // set padding bytes to 0 (setting as 0xfe for debugging)
    #if DEBUG
    memset(base, 0xfe, padding);
    #endif // DEBUG

    // update block info
    BlockInfo * newBlock = (BlockInfo *)(base + padding);
    newBlock->_padding = padding;
    newBlock->_dataSize = blockSize - padding - BlockInfoSize;

    if (isFirstFree)  _firstFree = newBlock;
    if (isHead)       _head = newBlock;
    if (isTail)       _tail = newBlock;

    linkBlocks(newBlock->_prev, newBlock, newBlock->_next);

    return newBlock;
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

    linkBlocks(block->_prev, block, newBlock, block->_next);

    return block;
}

MemMan::BlockInfo * MemMan::growBlock(BlockInfo * block, size_t biggerSize, size_t align) {
    guard_t guard{_mainMutex};

    #if DEBUG
    assert(block->isValid() && "Block not valid.");
    #endif // DEBUG

    // TODO: optimize for quick expansion if next block is free and big enough

    // next not free or not big enough
    // move the block to new location
    BlockInfo * newBlock = createBlock({
        .size = biggerSize,
        .align = align,
        .type = block->_type
    });
    copy(newBlock->data(), block->data());
    releaseBlock(block);
    assert(newBlock && "Not enough room to move block during grow.");

    return newBlock;
}

MemMan::BlockInfo * MemMan::resizeBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    if (block->_dataSize > _request->size) return shrinkBlock(block, _request->size);
    if (block->_dataSize < _request->size) return growBlock(block, _request->size, _request->align);

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG

    return block;
}

void MemMan::mergeAllAdjacentFreeBlocks() {
    guard_t guard{_mainMutex};

    _blockCount = 0;
    for (BlockInfo * bi = _head; bi; bi = bi->_next) {
        BlockInfo * newBi = mergeWithNextBlock(bi);
        if (newBi) bi = newBi;
        ++_blockCount;
    }

    #if DEBUG
    validateAllBlocks();
    #endif // DEBUG
}

void MemMan::findFirstFreeBlock(BlockInfo * block) {
    guard_t guard{_mainMutex};

    // if _firstFree is already earlier in list than suggested search starting point (block)
    // then abort, no search needed.
    if ((size_t)_firstFree < (size_t)block) {
        return;
    }

    for (BlockInfo * bi = block; bi; bi = bi->_next) {
        if (bi->_type == MEM_BLOCK_FREE) {
            _firstFree = bi;
            return;
        }
    }

    // should only happen if no free blocks exist
    _firstFree = nullptr;
}
