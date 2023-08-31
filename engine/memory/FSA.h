#pragma once
#include <assert.h>
#include "../common/debug_defines.h"
#include "../common/types.h"
#include "mem_utils.h"


/*

Fixed-size allocator.

Designed to be used within pre-allocated memory.

Handles memory requests that can fit small bytes sizes (2^1–2^12, or 2-4096).
Uses some basic POT tricks to quickly find previous allocs and free slots.
Designed to exist in a MemMan block, setup privately, and utilized to avoid
creating new blocks.

FSA data structure (within a MemMan block):
            data() start
            v
            | 2-byte group of sub-blocks (if present) || 4-byte group etc    ...
|__________||______________|__|__|__|__|__|__|__|__|__||______________|____|_...
FSA         Free list      Sub-blocks                  Free list      Sub-blk...
(class      (bit-array)
storage)    ^                                          ^
            alignment point                            align pt
                           ^                                          ^
                           alignment point                            align pt
*/

class FSA {
    // FRIENDS
public:
    friend class MemMan;

    // TYPES
public:
    using Setup = MemManFSASetup;
    constexpr static size_t Max = Setup::Max;
    static constexpr size_t MinBytes = 1;
    static constexpr size_t MaxBytes = 1 << Max;

    static constexpr size_t DataSize(Setup const & setup) {
        size_t size = 0;
        for (size_t i = 0; i < Max; ++i) {
            if (setup.nSubBlocks[i] == 0) {
                continue;
            }
            size +=
                FreeListByteSize(setup.nSubBlocks[i], setup.align) +
                alignSize(SubBlockByteSize(i) * setup.nSubBlocks[i], setup.align);
        }
        return size;
    }

    uint16_t subBlockCountForGroup(uint16_t groupIndex) const;
    byte_t const * freeListPtrForGroup(uint16_t groupIndex) const;
    bool isFree(uint16_t groupIndex, uint16_t subBlockIndex) const;
    // returns false if not found
    bool indicesForPtr(void * ptr, uint16_t * foundGroupIndex, uint16_t * foundSubBlockIndex) const;
    byte_t const * data() const;
    bool containsPtr(void * ptr) const;
    uint16_t sizeForPtr(void * ptr) const;

    // INTERNALS
private:
    FSA(Setup const & setup);
    void updateMap();
    byte_t * data();
    // try to claim sub-block of size. returns ptr if found.
    void * alloc(size_t size);
    // release sub-block at ptr
    bool destroy(void * ptr);
    byte_t * subBlockPtr(uint16_t groupIndex, uint16_t subBlockIndex);
    void setClaimed(byte_t * freeList, uint16_t subBlockIndex);
    void setFree(byte_t * freeList, uint16_t subBlockIndex);
    void setAllFree();
    void setAllFreeInGroup(uint16_t groupIndex);
    void printInfo();

    #if DEBUG
    void test();
    #endif // DEBUG

    // STORAGE
private:
    // pointers to subblock groups. _freeList[0] is pointer to 2-byte freelist, etc.
    // _groupBase[0] is pointer to first sub-block in 2-byte group, etc.
    // nullptr means group of that byte size does not exist.
    byte_t * _freeList[Max] = {nullptr};
    byte_t * _groupBase[Max] = {nullptr};
    // subblock counts. _nSubBlocks[0] is number of 2-byte subblocks, etc.
    uint16_t _nSubBlocks[Max] = {0};
    // total data size in bytes (does not include sizeof(FSA))
    size_t _dataSize = 0;
    // alignment
    size_t _align = 0;

    // STATIC INTERNALS
private:
    static uint16_t constexpr SubBlockByteSize(uint8_t groupIndex) {
        assert(groupIndex < Max && "Index out of range.");
        return (uint16_t)2 << groupIndex;
    }

    static uint16_t constexpr FreeListByteSize(uint16_t nSubBlocks, size_t align = 0) {
        assert(nSubBlocks == (nSubBlocks & 0xfff8) &&
            "Number of FSA subblocks must be a multiple of 8.");
        if (nSubBlocks == 0) return 0;
        return alignSize(((nSubBlocks - 1) >> 3) + 1, align);
    }

    static uint8_t constexpr IndexForByteSize(uint16_t byteSize) {
        assert(byteSize >= MinBytes && byteSize <= MaxBytes && "Invalid byte size.");
        byteSize -= 1;
        int index = 0;
        while (byteSize >>= 1) ++index;
        return index;
    }

    // DEV INTERFACE
private:
    #if DEV_INTERFACE
    void editorEditBlock();
    #endif // DEV_INTERFACE
};









/*

SOME TEST CODE:

printl("FSAMap min/max bytes %d—%d", FSAMap::MinBytes, FSAMap::MaxBytes);
FSAMap map;
for (int i = FSAMap::MinBytes; i <= FSAMap::MaxBytes; ++i) {
    int b = map.IndexForByteSize(i); // private
    int bs = map.ActualByteSizeForByteSize(i);
    printl("IndexForByteSize(%d) : %d, acutal block size: %d", i, b, bs);
}

*/


// class FSA {
// public:
//     friend class MemMan;

//     static size_t constexpr DataSize(uint16_t subBlockSize, uint16_t nSubBlocks) {
//         assert(nSubBlocks / 8 * 8 == nSubBlocks && "nSubBlocks must be multiple of 8.");
//         return
//             sizeof(FSA) +
//             BitArraySize(nSubBlocks) +
//             ActualSubBlockSize(subBlockSize) * nSubBlocks;
//     }

//     static size_t constexpr BitArraySize(uint16_t nSubBlocks) {
//         return nSubBlocks / 8 + (nSubBlocks % 8 != 0);
//     }

//     static size_t constexpr ActualSubBlockSize(uint16_t byteSize) {
//         uint8_t exp = IndexForByteSize(byteSize) + 1;
//         return (size_t)((uint16_t)1 << exp);
//     }

//     static uint8_t constexpr IndexForByteSize(uint16_t byteSize) {
//         assert(byteSize >= 1 && byteSize <= 4096 && "Invalid byte size.");
//         byteSize -= 1;
//         int index = 0;
//         while (byteSize >>= 1) ++index;
//         return index;
//     }

//     uint16_t subBlockSize() const { return _subBlockSize; }
//     uint16_t nSubBlocks() const { return _nSubBlocks; }

//     byte_t * bitArray() const {
//         return (byte_t *)this + sizeof(FSA);
//     }

//     bool isFree(uint16_t index) const {
//         assert(index < _nSubBlocks && "Index out of range.");
//         return ((bitArray()[index/8]) >> (index % 8)) & 1;
//     }

// private:
//     FSA(uint16_t subBlockSize, uint16_t nSubBlocks) {
//         assert(subBlockSize > 0 && "subBlockSize must be at least one byte");
//         assert(nSubBlocks  / 8 * 8 == nSubBlocks  && "nSubBlocks  must be multiple of 8.");
//         _subBlockSize = subBlockSize;
//         _nSubBlocks = nSubBlocks;
//     }

//     uint16_t _subBlockSize = 0;
//     uint16_t _nSubBlocks = 0;
// };






    /*
    Map designed to lookup FSA locations in memory given requested byte size.

    Supports 12 diffent FSA blocks from of subblock sizes 2^1 (2) to 2^12 (4096).

        // static int constexpr MinBytes = 1;    // smallest block is 2^1, 1 byte request uses 2-byte blocks
        // static int constexpr MaxBytes = 4096; // 2^MaxBlocks
        // static int constexpr MaxBlocks = 12;

        // byte_t * activePtrForByteSize(uint16_t byteSize) {
        //     uint8_t ideal = IndexForByteSize(byteSize);
        //     while (ideal < MaxBlocks && _ptrs[ideal] == nullptr) {
        //         ++ideal;
        //     }
        //     if (ideal == MaxBlocks) return nullptr;
        //     return _ptrs[ideal];

        // }

        // byte_t * ptrForByteSize(uint16_t byteSize) {
        //     return _ptrs[IndexForByteSize(byteSize)];
        // }

        // void setPtrForByteSize(uint16_t byteSize, byte_t * ptr) {
        //     _ptrs[IndexForByteSize(byteSize)] = ptr;
        // }

        // bool hasPtrForByteSize(uint16_t byteSize) {
        //     return (_ptrs[IndexForByteSize(byteSize)] != nullptr);
        // }




    class Map {
    private:

        // returns 0-11, given byteSize range MinBytes-MaxBytes
        static uint8_t constexpr IndexForByteSize(uint16_t byteSize) {
            assert(byteSize >= MinBytes && byteSize <= MaxBytes && "Invalid byte size.");
            byteSize -= 1;
            int index = 0;
            while (byteSize >>= 1) ++index;
            return index;
        }
    };

    */
