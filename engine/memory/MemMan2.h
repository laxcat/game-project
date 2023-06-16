#pragma once
#include <mutex>
#include "../engine.h"
#include "../common/types.h"
#include "FSA.h"

/*

MemMan _data --->
Block 1                                                Block 2
|__________|______________|___________________________||____________...
_padding   BlockInfo      BlockInfo->data()            _padding
                          (custom class)               (next block)
                          ^
                          alignment point
*/


class MemMan2 {
    // TYPES
public:
    using guard_t = std::lock_guard<std::recursive_mutex>;

    // "MemBlock"
    #define BLOCK_MAGIC_STRING2 {0x4D, 0x65, 0x6D, 0x42, 0x6C, 0x6F, 0x63, 0x6B}
    constexpr static byte_t BlockMagicString[8] = BLOCK_MAGIC_STRING2; // "MemBlock"

    class BlockInfo {
        // API
    public:
        // lookups
        size_t paddingSize() const;
        size_t dataSize() const;
        size_t blockSize() const;
        MemBlockType type() const;
        byte_t const * data() const;
        byte_t const * basePtr() const;

        // modifiers
        byte_t * data();

        // STORAGE
    private:
        size_t _dataSize = 0;
        BlockInfo * _next;
        BlockInfo * _prev;
        uint32_t _padding = 0;
        MemBlockType _type = MEM_BLOCK_FREE;

        #if DEBUG
        size_t _debug_index = SIZE_MAX;
        byte_t _debug_magic[8] = BLOCK_MAGIC_STRING2;
        #endif // DEBUG

        // INTERNALS
    private:
        // align functions
        byte_t const * calcUnalignedDataLoc() const;
        size_t calcAlignPaddingSize(size_t align) const;
        size_t calcAlignDataSize(size_t align) const;
        bool isAligned(size_t align) const;
        byte_t * basePtr();

        // debug only
        #if DEBUG
        bool isValid() const;
        #endif // DEBUG

        // FRIENDS
    public:
        friend class MemMan2;
    };
    constexpr static size_t BlockInfoSize = sizeof(BlockInfo);

    // API
public:
    void init(EngineSetup const & setup);
    void startFrame(size_t frame);
    void endFrame();
    void shutdown();

    byte_t const * data() const;
    size_t size() const;
    BlockInfo * firstBlock() const;
    BlockInfo * nextBlock(BlockInfo const * block) const;
    size_t blockCountForDisplayOnly() const;

    // generic alloc request, which can return Block or pointer within FSA
    void * alloc(size_t size, size_t align = 0, BlockInfo ** resultBlock = nullptr);
    // finds free block of size
    BlockInfo * request(size_t size, size_t align = 0);
    // set type to free and reset padding
    BlockInfo * release(BlockInfo * block);

    bool destroy(void * ptr);

    // SPECIAL BLOCK CREATION
private:
    // create fsa block on init
    FSA * createFSA(MemManFSASetup const & setup);

    // STORAGE
private:
    byte_t * _data = nullptr;
    size_t _size = 0;
    BlockInfo * _head = nullptr;
    BlockInfo * _tail = nullptr;
    BlockInfo * _firstFree = nullptr;
    FSA * _fsa = nullptr;
    size_t _frame = 0;
    size_t _blockCount = 0; // updated during end frame, for display purposes only
    mutable std::recursive_mutex _mainMutex;

    // INTERNALS
private:
    // alters _padding and _dataSize to align data() to alignment
    BlockInfo * claim(BlockInfo * block, size_t size, size_t align = 0);
    // consumes next block if free
    BlockInfo * mergeWithNext(BlockInfo * block);
    // realigns block in place
    BlockInfo * realign(BlockInfo * block, size_t relevantDataSize, size_t align);
    // creates free block if possible
    BlockInfo * shrink(BlockInfo * block, size_t smallerSize);
    // consumes next free block, or moves block
    BlockInfo * grow(BlockInfo * block, size_t biggerSize, size_t align = 0);
    // shortcut to grow/shrink
    BlockInfo * resize(BlockInfo * block, size_t newSize, size_t align = 0);
    // combine adjacent free blocks
    void mergeAllAdjacentFree();
    // scan forward to find first free
    void findFirstFree(BlockInfo * block);
    // BlockInfo for raw ptr pointing to data.
    BlockInfo * blockForPtr(void * ptr);

    // // traverse nodes backwards to find first free
    // NOTE: WE SHOULDN'T NEED THIS IF HANDLED PROPERLY IN RELEASE
    // void updateFirstFree();

    #if DEBUG
    // validate all blocks, update debug info like _debug_index
    void validateAll();
    #endif // DEBUG
};
