#pragma once
#include <mutex>
#include <bx/allocator.h>
#include "../engine.h"
#include "../common/types.h"
#include "../common/debug_defines.h"

// special block-types that need to be fully included
#include "Array.h"
#include "Pool.h"
#include "Gobj.h"

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
// special block-types that can be forward declared
class CharKeys;
class FSA;
class File;
class FrameStack;
class FreeList;

class MemMan {
// FRIENDS ------------------------------------------------------------------ //
public:
    friend class BXAllocator;


// TYPES -------------------------------------------------------------------- //
public:
    using guard_t = std::lock_guard<std::recursive_mutex>;

    #if DEBUG
    //                          M     e     m     B     l     o     c     k
    #define BLOCK_MAGIC_STRING {0x4D, 0x65, 0x6D, 0x42, 0x6C, 0x6F, 0x63, 0x6B}
    constexpr static byte_t BlockMagicString[8] = BLOCK_MAGIC_STRING;
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

        // usually only checked in debug, but can help validate an unknown
        // pointer to block conversion, so has release uses too
        bool isValid() const;

        // debug only
        #if DEBUG
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
        MemBlockType type = MEM_BLOCK_GENERIC;

        // number of frames allocation should last. -1 == forever (until manual release)
        // common usage: set to 0 for auto-release at end of current frame
        int lifetime = -1;

        // request from tail of block
        bool high = false;
    };

    class Result {
    public:
        size_t size = 0;
        size_t align = 0;
        void * ptr = nullptr;
        BlockInfo * block = nullptr;
    };

    class AutoRelease {
    public:
        void * ptr;
        int lifetime = 1;
    };


// PUBLIC INTERFACE --------------------------------------------------------- //
public:
    // LIFECYCLE
    void init(EngineSetup const & setup, FrameStack ** frameStack = nullptr);
    void startFrame(size_t frame);
    void endFrame();
    void shutdown();

    // ACCESS
    byte_t const * data() const;
    size_t size() const;
    BlockInfo * firstBlock() const;
    BlockInfo * nextBlock(BlockInfo const * block) const;
    size_t blockCountForDisplayOnly() const;
    bool isPtrInFSA(void * ptr) const;

    // GENERIC ALLOCATION
    // copys request param into request block, then executes request
    void * request(Request const & newRequest);
    template <typename T, typename ... TS>
    T * create(TS && ... params);

    // SPECIAL BLOCK OBJ CREATION
    // MEM_BLOCK_ARRAY
    template<typename T>
    Array<T> * createArray(size_t max);
    // MEM_BLOCK_CHARKEYS
    CharKeys * createCharKeys(size_t max);
    // MEM_BLOCK_FILE
    File * createFileHandle(char const * path, bool loadNow, Request const & addlRequest);
    // MEM_BLOCK_FRAMESTACK
    FrameStack * createFrameStack(size_t size);
    // MEM_BLOCK_FREELIST
    FreeList * createFreeList(size_t max);
    // MEM_BLOCK_GOBJ
    Gobj * createGobj(char const * gltfPath);
    Gobj * createGobj(Gobj::Counts const & counts, int lifetime = -1);
    // MEM_BLOCK_POOL
    template<typename T>
    Pool<T> * createPool(size_t max);


// SPECIAL INIT BLOCK OBJ CREATION ------------------------------------------ //
private:
    // create request block on init
    void createRequestResult();
    // create fsa block on init
    FSA * createFSA(MemManFSASetup const & setup);
    // create auto-release buffer on init
    Array<MemMan::AutoRelease> * createAutoReleaseBuffer(size_t size);


// STORAGE ------------------------------------------------------------------ //
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
    Array<AutoRelease> * _autoReleaseBuffer = nullptr;
    #if DEBUG
    size_t _frame = 0;
    #endif // DEBUG
    size_t _blockCount = 0; // updated during end frame, for display purposes only
    mutable std::recursive_mutex _mainMutex;


// INTERNALS ---------------------------------------------------------------- //
private:
    // execute request as found in request block; combined alloc/realloc/free
    void request();
    // explicitly finds/creates free block of size. reads params from most recent Request object.
    BlockInfo * createBlock();
    BlockInfo * createBlock(Request const & request);
    // explicitly releases block. set type to free and reset padding
    void releaseBlock(BlockInfo * block);
    // alters _padding and _dataSize to align data() to alignment, reads params from most recent Request object.
    BlockInfo * claimBlock(BlockInfo * block);
    // same as claim but splits off back of block instead of front
    BlockInfo * claimBlockBack(BlockInfo * block);
    // consumes next block if free
    BlockInfo * mergeWithNextBlock(BlockInfo * block);
    // link prev/next for internal linkage of list of blocks. (will not set blockList[0]->_prev, etc)
    // first/last blocks in list might be nullptr if just removed, so adjacent blocks can update links.
    void linkBlockList(BlockInfo ** blockList, uint16_t nBlocks);
    template <typename ... T>
    void linkBlocks(T && ... blocks);
    // realigns block in place
    BlockInfo * realignBlock(BlockInfo * block, size_t align);
    // creates free block if possible
    BlockInfo * shrinkBlock(BlockInfo * block, size_t smallerSize);
    // consumes next free block, or moves block
    BlockInfo * growBlock(BlockInfo * block, size_t biggerSize, size_t align = 0);
    // shortcut to grow/shrink
    BlockInfo * resizeBlock(BlockInfo * block);
    // copy data from any block/FSA location to another block/FSA location
    void copy(void * dst, void * src);
    // combine adjacent free blocks
    void mergeAllAdjacentFreeBlocks();
    // scan forward to find first free
    void findFirstFreeBlock(BlockInfo * block);
    // BlockInfo for raw ptr pointing to data.
    BlockInfo * blockForPtr(void * ptr);
    BlockInfo const * blockForPtr(void * ptr) const;
    // is ptr within MemMan's data?
    bool containsPtr(void * ptr) const;
    // find size of FSA-sub-block or block of pointer target
    size_t sizeOfPtr(void * ptr, BlockInfo const ** block = nullptr) const;
    // handle lifetime of current AutoRelease allocations at end of every frame
    void autoReleaseEndFrame();
    // conditionally add auto-release
    void addAutoRelease();
    // conditionally update auto-release ptr
    void updateAutoRelease();
    // conditionally remove auto-release
    void removeAutoRelease();


// DEV INTERFACE ------------------------------------------------------------ //
#if DEV_INTERFACE
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


// DEBUG ONLY --------------------------------------------------------------- //
public:
    #if DEBUG
    // validate all blocks, update debug info like _debug_index
    void validateAllBlocks();
    // print all blocks
    void printAllBlocks() const;
    // print free blocks
    void printFreeBlockSizes() const;
    // print request and result
    void printRequestResult() const;
    void printRequest() const;
    void printResult() const;
    void printPtr(void * ptr, uint16_t indent = 0) const;
    #endif // DEBUG
};


// ALLOCATORS --------------------------------------------------------------- //

class BXAllocator : public bx::AllocatorI {
public:
    MemMan * memMan = nullptr;
    void * realloc(void *, size_t, size_t, char const *, uint32_t);
};


// INLINE DECLARATIONS ------------------------------------------------------ //

template<typename T>
Array<T> * MemMan::createArray(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(Array<T>) + max * sizeof(T)});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_ARRAY;
    return new (block->data()) Array<T>{max};
}

template<typename T>
Pool<T> * MemMan::createPool(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(Pool<T>) + Pool<T>::DataSize(size)});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_POOL;
    return new (block->data()) Pool<T>{size};
}

template <typename T, typename ... TS>
T * MemMan::create(TS && ... params) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({.size = sizeof(T)});
    if (!block) return nullptr;
    block->_type = MEM_BLOCK_GENERIC;
    return new (block->data()) T{static_cast<TS &&>(params)...};
}

template <typename... TS>
void MemMan::linkBlocks(TS && ... blocks) {
    static constexpr size_t N = sizeof...(blocks);
    BlockInfo * blockList[N] = {blocks...};
    linkBlockList(blockList, N);
}
