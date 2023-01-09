#pragma once
#include "../common/types.h"
#include <stdio.h>
#include <bx/allocator.h>
#include "File.h"
#include "Pool.h"
#include "Stack.h"

class MemMan {
public:
    // types ---------------------------------------------------------------- //

    enum Type {
        // empty space to be claimed
        TYPE_FREE,
        // internal types, with special treatment
        TYPE_POOL,
        TYPE_STACK,
        TYPE_FILE,
        TYPE_ENTITY,
        // externally requested of any type
        TYPE_EXTERNAL
    };

    // Block, basic unit of all sub sections of the memory space.
    //
    // Each block will be placed in the large memory space, with an instance
    // placed at the start of the block, with "size" bytes available directly
    // after in memory. Member "size" represents this data space size, so the
    // block actually takes up size + BlockInfoSize in MemMan data block.
    class Block {
    public:
        friend class MemMan;
        friend void * memManAlloc(size_t, void *);

        size_t dataSize() const { return size; }
        size_t totalSize()  const { return BlockInfoSize + size; }
        byte_t * data() { return (byte_t *)this + BlockInfoSize; }

    private:
        size_t size = 0; // this is data size, not total size!
        Block * prev = nullptr;
        Block * next = nullptr;
        Type type = TYPE_FREE;

        // "info" could maybe be a magic string (for safety checks). or maybe additional info.
        // 8-byte allignment on personal development machine is forcing this Block to always be
        // 32 bytes anyway so this is here to make that explicit. We could even take more space
        // from type, which could easily be only 1 or 2 bytes.
        byte_t info[4];

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
    // find block with enough free space, split it to size, and return it
    Block * requestFreeBlock(size_t size);
    // split block A into block A (with requested data size) and block B with what remains.
    Block * splitBlock(Block * b, size_t blockANewSize);
    // same as split but sizes and returns second block
    Block * splitBlockEnd(Block * b, size_t blockBNewSize);
    // merge this block with next block IF both are free
    Block * mergeBlockWithNext(Block * b);
    // resize block if possible
    Block * resizeBlock(Block * b, size_t newSize);

    // checks and assertions ------------------------------------------------ //
    // check if random pointer is within managed range
    void assertWithinData(void * ptr, size_t size = 0);

    // storage -------------------------------------------------------------- //
private:
    byte_t * _data = nullptr;
    Block * _blockHead = nullptr;
    Block * _blockTail = nullptr;
    size_t _size = 0;

    // debug ---------------------------------------------------------------- //

public:
    void getInfo(char * buf, int bufSize);
};

template <typename T, typename ... TP>
inline T * MemMan::create(size_t size, TP && ... params) {
    Block * block = requestFreeBlock(size + sizeof(T));
    if (!block) return nullptr;
    block->type = TYPE_EXTERNAL;
    return new (block->data()) T{static_cast<TP &&>(params)...};
}

void * memManAlloc(size_t size, void * userData);
void * memManRealloc(void * ptr, size_t size, void * userData);
void memManFree(void * ptr, void * userData);

class BXAllocator : public bx::AllocatorI {
public:
    MemMan * memMan = nullptr;
    void * realloc(void *, size_t, size_t, char const *, uint32_t);
};
