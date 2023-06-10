#pragma once
#include <mutex>
#include "../engine.h"
#include "../common/types.h"

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

    // "MemB"
    #define BLOCK_MAGIC_STRING {0x4D, 0x65, 0x6D, 0x42}
    constexpr static byte_t BlockMagicString[4] = BLOCK_MAGIC_STRING; // "MemB"

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
        byte_t _debug_magic[4] = BLOCK_MAGIC_STRING;
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

    // STORAGE
private:
    byte_t * _data = nullptr;
    size_t _size = 0;
    BlockInfo * _head = nullptr;
    BlockInfo * _tail = nullptr;
    BlockInfo * _firstFree = nullptr;
    size_t _frame = 0;
    mutable std::recursive_mutex _mainMutex;

    // API
public:
    void init(EngineSetup const & setup);
    void startFrame(size_t frame);
    void endFrame();
    void shutdown();
    BlockInfo * firstBlock() const;
    BlockInfo * nextBlock(BlockInfo const * block) const;

    // finds free block of size
    BlockInfo * request(size_t size, size_t align = 0);

    // INTERNALS
private:
    // alters _padding and _dataSize to align data() to alignment
    BlockInfo * claim(BlockInfo * block, size_t size, size_t align = 0);
    // set type to free and reset padding
    BlockInfo * release(BlockInfo * block);
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
    // traverse nodes backwards to find first free
    void updateFirstFree();

    #if DEBUG
    // validate all blocks, update debug info like _debug_index
    void validateAll();
    #endif // DEBUG
};
