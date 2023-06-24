#pragma once
#include <mutex>
#include <bx/allocator.h>
#include "../engine.h"
#include "../common/types.h"
#include "FSA.h"
#include "Stack.h"

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
    // FRIENDS
public:
    friend void * memManAlloc(size_t, void *, size_t);
    friend void * memManRealloc(void *, size_t, void *, size_t);
    friend void memManFree(void *, void *);
    friend class BXAllocator;

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
        bool contains(void * ptr, size_t size) const;

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
        void print(size_t index = SIZE_MAX) const;
        #endif // DEBUG

        // FRIENDS
    public:
        friend class MemMan2;
        friend class BXAllocator;
        friend void * memManAlloc(size_t, void *, size_t);
        friend void * memManRealloc(void *, size_t, void *, size_t);
    };
    constexpr static size_t BlockInfoSize = sizeof(BlockInfo);

    class Request {
    public:
        size_t size = 0;
        size_t align = 0;
        void * ptr = nullptr;
        BlockInfo * copyFrom = nullptr;
        MemBlockType type = MEM_BLOCK_NONE;
    };
    class Result {
    public:
        size_t size = 0;
        size_t align = 0;
        void * ptr = nullptr;
        BlockInfo * block = nullptr;
    };

    // API
public:
    // LIFECYCLE
    void init(EngineSetup const & setup, Stack ** frameStack = nullptr);
    void startFrame(size_t frame);
    void endFrame();
    void shutdown();

    // ACCESS
    byte_t const * data() const;
    size_t size() const;
    BlockInfo * firstBlock() const;
    BlockInfo * nextBlock(BlockInfo const * block) const;
    size_t blockCountForDisplayOnly() const;

    // GENERIC ALLOCATION
    // // generic alloc request, which can return Block data ptr or ptr within FSA
    // void * alloc(size_t size, size_t align = 0, BlockInfo ** resultBlock = nullptr);
    // // release generic pointer; expects block->data() ptr or FSA sub-block ptr
    // bool destroy(void * ptr);
    // copys request param into request block, then executes request
    void * request(Request const & newRequest);

    // SPECIAL BLOCK OBJ CREATION
    Stack * createStack(size_t size);

    // DEV INTERFACE ONLY
    #if DEV_INTERFACE
    void editor();
    #endif // DEV_INTERFACE

    // PRIVATE SPECIAL BLOCK OBJ CREATION
private:
    // create request block on init
    void createRequestResult();
    // create fsa block on init
    FSA * createFSA(MemManFSASetup const & setup);

    // STORAGE
private:
    byte_t * _data = nullptr;
    size_t _size = 0;
    BlockInfo * _head = nullptr;
    BlockInfo * _tail = nullptr;
    BlockInfo * _firstFree = nullptr;
    Request * _request = nullptr;
    Result * _result = nullptr;
    FSA * _fsa = nullptr;
    BlockInfo * _fsaBlock = nullptr;
    size_t _frame = 0;
    size_t _blockCount = 0; // updated during end frame, for display purposes only
    mutable std::recursive_mutex _mainMutex;

    // INTERNALS
private:
    // execute request as found in request block
    void request();
    // explicitly finds/creates free block of size
    BlockInfo * create(size_t size, size_t align = 0, BlockInfo * copyFrom = nullptr);
    // explicity releases block. set type to free and reset padding
    BlockInfo * release(BlockInfo * block);
    // alters _padding and _dataSize to align data() to alignment
    BlockInfo * claim(BlockInfo * block, size_t size, size_t align = 0, BlockInfo * copyFrom = nullptr);
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
    // print all blocks
    void printAll() const;
    // print free blocks
    void printFreeSizes() const;
    // print request and result
    void printRequestResult() const;
    #endif // DEBUG
};

// requires size (and userData must be pointer to MemMan)
void * memManAlloc(size_t size, void * userData, size_t align = 0);
// general purpose to handle all cases. requires ptr OR size (and userData must be pointer to MemMan)
void * memManRealloc(void * ptr, size_t size, void * userData, size_t align = 0);
// requires ptr (and userData must be pointer to MemMan)
void memManFree(void * ptr, void * userData);

class BXAllocator : public bx::AllocatorI {
public:
    MemMan2 * memMan2 = nullptr;
    void * realloc(void *, size_t, size_t, char const *, uint32_t);
};
