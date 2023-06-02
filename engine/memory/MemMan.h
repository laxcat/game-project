#pragma once
#include "../common/types.h"
#include <stdio.h>
#include <bx/allocator.h>
#include <mutex>
#include "File.h"
#include "Pool.h"
#include "Stack.h"
#include "../common/debug_defines.h"

class MemMan {
public:
    // friends -------------------------------------------------------------- //
    friend void * memManAlloc(size_t, void *);
    friend void * memManRealloc(void *, size_t, void *);
    friend void memManFree(void *, void *);
    friend class BXAllocator;

    // types ---------------------------------------------------------------- //
    enum Type {
        // unsued
        TYPE_FREE,    // empty space to be claimed
        TYPE_CLAIMED, // claimed but type not set yet
        // internal types, with special treatment
        TYPE_POOL,
        TYPE_STACK,
        TYPE_FILE,
        TYPE_ENTITY,
        // special blocks for small allocations?
        // TYPE_SMALL_ALLOC
        // requested by BGFX. (no special treatment atm)
        TYPE_BGFX,
        // externally requested of any type
        TYPE_EXTERNAL
    };

    using guard_t = std::lock_guard<std::recursive_mutex>;

    // "MemB"
    #define BLOCK_MAGIC_STRING {0x4D, 0x65, 0x6D, 0x42}
    constexpr static byte_t BlockMagicString[4] = BLOCK_MAGIC_STRING; // "MemB"

    // Block, basic unit of all sub sections of the memory space.
    //
    // Each block will be placed in the large memory space, with an instance
    // placed at the start of the block, with "size" bytes available directly
    // after in memory. Member "size" represents this data space size, so the
    // block actually takes up size + BlockInfoSize in MemMan data block.
    class Block {
    public:
        friend class MemMan;
        friend class BXAllocator;
        friend void * memManAlloc(size_t, void *);

        size_t   dataSize()  const  { return _dataSize; }
        size_t   totalSize() const  { return BlockInfoSize + _dataSize; }
        Type     type()      const  { return _type; }
        byte_t * data()             { return (byte_t *)this + BlockInfoSize; }
        byte_t const * data() const { return (byte_t const *)((byte_t *)this + BlockInfoSize); }

    private:
        size_t _dataSize = 0;
        Block * _next = nullptr;
        Type _type = TYPE_FREE;

        // "info" could maybe be a magic string (for safety checks). or maybe additional info.
        // 8-byte allignment on personal development machine is forcing this Block to always be
        // 32 bytes anyway so this is here to make that explicit. We could even take more space
        // from type, which could easily be only 1 or 2 bytes.
        // INGORE ABOVE. setting magic string for now:
        // magic string: MemB
        byte_t _info[4] = BLOCK_MAGIC_STRING;

        // expand Block for debug purposes
        #if DEBUG
        size_t debug_index = SIZE_MAX;
        #endif // DEBUG

        Block() {}
    };
    constexpr static size_t BlockInfoSize = sizeof(Block);

    // info/access ---------------------------------------------------------- //
    size_t size() const { return _size; }

    // lifecycle ------------------------------------------------------------ //
    void init(size_t size);
    void shutdown();

    // create/destroy for internal types ------------------------------------ //
    Pool * createPool(size_t objCount, size_t objSize);
    void destroyPool(Pool * a);
    Stack * createStack(size_t size);
    void destroyStack(Stack * s);
    File * createFileHandle(char const * path, bool loadNow = false);
    void destroyFileHandle(File * f);
    // Entity * createEntity(char const * path, bool loadNow = false);
    // void destroyEntity(Entity * f);
    template <typename T, typename ... TP>
        T * create(size_t size, TP && ... params);
    void destroy(void * ptr);

    // block handling ------------------------------------------------------- //
    // find block with enough free space
    Block * findFree(size_t size);
    // find block with enough free space, split it to size, and return it
    Block * requestFreeBlock(size_t size);
    // split block A into block A (with requested data size) and block B with what remains.
    Block * splitBlock(Block * b, size_t blockANewSize);
    // same as split but sizes and returns second block
    Block * splitBlockEnd(Block * b, size_t blockBNewSize);
    // merge this block with next block IF both are free
    Block * mergeFreeBlocks(Block * b);
    // resize block if possible
    Block * resizeBlock(Block * b, size_t newSize);

    // return Block for generic ptr, where ptr is expected to be at block->data()
    Block * blockForDataPtr(void * ptr);

    // block reading -------------------------------------------------------- //
    Block const * firstBlock() const;
    Block const * nextBlock(Block const & block) const;

    // checks and assertions ------------------------------------------------ //
    // check if random pointer is within managed range
    bool isWithinData(void * ptr, size_t size = 0) const;

    bool checkAllBlocks();

    // storage -------------------------------------------------------------- //
private:
    byte_t * _data = nullptr;
    Block * _blockHead = nullptr;
    // Block * _blockTail = nullptr;
    size_t _size = 0;
    mutable std::recursive_mutex mainMutex;

    // internal ------------------------------------------------------------- //
private:
    #if DEBUG
    // create index for all in linked list for debuging purposes
    void reindexBlocks();
    #endif // DEBUG
    // used for interal debug assertions
    bool isValidBlock(Block * block) const;

    // debug ---------------------------------------------------------------- //
public:
    void getInfo(char * buf, int bufSize);
    void printInfo(char const * prefixMsg = nullptr);
};

template <typename T, typename ... TP>
inline T * MemMan::create(size_t size, TP && ... params) {
    Block * block = requestFreeBlock(size + sizeof(T));
    if (!block) return nullptr;
    block->_type = TYPE_EXTERNAL;
    return new (block->data()) T{static_cast<TP &&>(params)...};
}


// requires size (and userData must be pointer to MemMan)
void * memManAlloc(size_t size, void * userData);
// general purpose to handle all cases. requires ptr OR size (and userData must be pointer to MemMan)
void * memManRealloc(void * ptr, size_t size, void * userData);
// requires ptr (and userData must be pointer to MemMan)
void memManFree(void * ptr, void * userData);

class BXAllocator : public bx::AllocatorI {
public:
    MemMan * memMan = nullptr;
    void * realloc(void *, size_t, size_t, char const *, uint32_t);
};

