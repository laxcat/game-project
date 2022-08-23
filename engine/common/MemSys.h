#pragma once
#include "types.h"

class MemSys {
public:
    // types ---------------------------------------------------------------- //

    enum Type {
        FREE,
        POOL,
    };

    // Block, basic unit of all sub sections of the memory space.
    //
    // Each block will be placed in the large memory space, with an instance
    // placed at the start of the block, with "size" bytes available directly
    // after in memory. Member "size" represents this data space size, so the
    // block actually takes up size + BlockInfoSize in MemSys data block.
    class Block {
    public:
        friend class MemSys;

        size_t dataSize() const { return size; }
        size_t totalSize()  const { return BlockInfoSize + size; }
        byte_t * data() { return (byte_t *)this + BlockInfoSize; }

    private:
        size_t size = 0; // this is data size, not total size!
        Block * prev = nullptr;
        Block * next = nullptr;
        Type type = FREE;
        byte_t info[4];
        Block() {}

        // split block A into block A (with requested data size) and block B with what remains.
        Block * split(size_t blockANewSize) {
            // block a (this) is not big enough.
            // equal data size also rejected because new block would have
            // 0 bytes for data.
            if (dataSize() <= blockANewSize + BlockInfoSize) return nullptr;

            // where to put the new block?
            byte_t * newLoc = data() + blockANewSize;
            // write it to data with defaults
            Block * b = new (newLoc) Block();
            // block b gets remaining space
            b->size = dataSize() - BlockInfoSize - blockANewSize;

            // set this size
            size = blockANewSize;

            // link up
            b->next = next;
            b->prev = this;
            next = b;

            return b;
        }

        // merge this block with next block IF both are free
        Block * mergeWithNext() {
            if (type != FREE || !next || next->type != FREE) return nullptr;
            size += next->totalSize();
            next = next->next;
            return this;
        }
    };
    constexpr static size_t BlockInfoSize = sizeof(Block);

    // block types (things we put into the blocks) -------------------------- //

    class Pool {
    public:
        friend class MemSys;

        size_t size() const { return objMaxCount * objSize; }

        template <typename T>
        T * claimTo(size_t index) {
            assert(sizeof(T) == objSize);
            if (index >= objMaxCount) return nullptr;
            freeIndex = index;
            return &((T *)ptr)[freeIndex];
        }

        template <typename T>
        T * claim(size_t count) {
            assert(sizeof(T) == objSize);
            if (freeIndex + count >= objMaxCount) return nullptr;
            freeIndex += count;
            return &((T *)ptr)[freeIndex];
        }

        void reset() { freeIndex = 0; }

    private:
        byte_t * ptr = nullptr;
        size_t objMaxCount = 0;
        size_t objSize = 0;
        size_t freeIndex = 0;

        Pool(byte_t * ptr, size_t objCount, size_t objSize) {
            this->ptr = ptr;
            this->objMaxCount = objCount;
            this->objSize = objSize;
        }
    };

    // Allocator

    // Stack

    // lifecycle ------------------------------------------------------------ //

    void init(size_t size) {
        _data = (byte_t *)malloc(size);
        _size = size;
        _blockHead = new (_data) Block();
        _blockHead->size = size - BlockInfoSize;
    }

    void shutdown() {
        if (_data) free(_data), _data = nullptr;
    }

    // create/destroy for block types --------------------------------------- //

    Pool * createPool(size_t objCount, size_t objSize) {
        Block * block = requestFreeBlock(objCount * objSize + sizeof(Pool));
        if (!block) return nullptr;
        block->type = POOL;
        return new (block->data()) Pool{block->data() + sizeof(Pool), objCount, objSize};
    }

    void destroyPool(Pool * a) {
        if (!isWithinData((byte_t *)a)) {
            fprintf(
                stderr,
                "Unexpected location for allocator. Expected mem range %p-%p, recieved %p",
                _data, _data + _size, a
            );
            assert(false);
        }

        // we verified the pool is in our expected memory space,
        // so it's safe to extract the block info just prior to it
        Block * block = (Block *)((byte_t *)a - BlockInfoSize);
        // set to free
        block->type = FREE;
        // try to merge with neighboring blocks
        block->mergeWithNext();
        if (block->prev) block->prev->mergeWithNext();

    }

private:
    byte_t * _data = nullptr;
    Block * _blockHead = nullptr;
    size_t _size = 0;

    // find block with enough free space, split it to size, and return it
    Block * requestFreeBlock(size_t size) {
        Block * block = nullptr;
        for (block = _blockHead; block; block = block->next) {
            if (block->type == FREE && block->dataSize() >= size) {
                break;
            }
        }
        if (!block) return nullptr;

        // we don't care if the split actually happens or not
        block->split(size);

        // the old block gets returned. it was (probably) shrunk to fit by split
        return block;
    }

    // check if random pointer is within
    bool isWithinData(byte_t * ptr, size_t size = 0) {
        return (ptr >= _data && ptr + size <= _data + _size);
    }

public:
    void getInfo(char * buf, int bufSize) {
        int blockCount = 0;
        for (Block * b = _blockHead; b; b = b->next) {
            ++blockCount;
        }

        int wrote = 0;
        wrote += snprintf(
            buf + wrote,
            bufSize - wrote,
            "MemSys\n"
            "================================================================================\n"
            "Data: %p, Size: %zu, Block count: %d\n"
            ,
            _data,
            _size,
            blockCount
        );

        int i = 0;
        for (Block * b = _blockHead; b != nullptr; b = b->next, ++i) {
            wrote += snprintf(
                buf + wrote,
                bufSize - wrote,
                "--------------------------------------------------------------------------------\n"
                "Block %d, %s\n"
                "--------------------\n"
                "Base: %p, Data: %p, Data size: %zu\n"
                "--------------------------------------------------------------------------------\n"
                ,
                i,
                    (b->type == FREE) ? "FREE" :
                    (b->type == POOL) ? "POOL" :
                    "(unknown type)"
                ,
                b,
                b->data(),
                b->dataSize()
            );
        }
    }
};
