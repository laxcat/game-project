#pragma once
#include <mutex>
#include <bx/allocator.h>
#include "../engine.h"
#include "../common/types.h"
#include "../common/debug_defines.h"
#include "Array.h"

/*

MemMan _data --->
Block 1                                                Block 2
|__________|______________|___________________________||____________...
_padding   BlockInfo      BlockInfo->data()            _padding
                          (custom class)               (next block)
                          ^
                          alignment point
*/

// forward declarations
class FSA;
class Stack;
class File;

class MemMan {
    // FRIENDS
public:
    friend class BXAllocator;

    // TYPES
public:
    using guard_t = std::lock_guard<std::recursive_mutex>;

    // "MemBlock"
    #if DEBUG
    #define BLOCK_MAGIC_STRING {0x4D, 0x65, 0x6D, 0x42, 0x6C, 0x6F, 0x63, 0x6B}
    constexpr static byte_t BlockMagicString[8] = BLOCK_MAGIC_STRING; // "MemBlock"
    #endif // DEBUG

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
        byte_t _debug_magic[8] = BLOCK_MAGIC_STRING;
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
        friend class MemMan;
        friend class BXAllocator;
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
    // copys request param into request block, then executes request
    void * request(Request const & newRequest);

    // SPECIAL BLOCK OBJ CREATION
    Stack * createStack(size_t size);
    File * createFileHandle(char const * path, bool loadNow);
    template<typename T>
    Array<T> * createArray(size_t max);

    #if DEV_INTERFACE
    // DEV INTERFACE ONLY
public:
    struct TestAlloc {
        constexpr static size_t DescSize = 64;
        char desc[DescSize];
        void * ptr = nullptr;
    };

    static constexpr size_t MaxTestAllocs = 128;

    void editor();
    void addTestAlloc(void * ptr, char const * formatString = NULL, ...);
    void removeAlloc(uint16_t i);
    void removeAllAllocs();

    TestAlloc testAllocs[MaxTestAllocs] = {};
    uint16_t nTestAllocs = 0;

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
    void release(BlockInfo * block);
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

class BXAllocator : public bx::AllocatorI {
public:
    MemMan * memMan = nullptr;
    void * realloc(void *, size_t, size_t, char const *, uint32_t);
};

// INLINE DECLARATIONS
template<typename T>
Array<T> * MemMan::createArray(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = create(sizeof(Array<T>) + max * sizeof(T));
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_ARRAY;
    return new (block->data()) Array<T>{max};
}
