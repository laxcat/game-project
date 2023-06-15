#pragma once
#include "../common/types.h"
#include "../dev/print.h"
#include "mem_utils.h"


/*

FSA data structure (within block)

            | 2-byte sub-block section (if present)   || 4-byte section etc  ...
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
    friend class MemMan2;

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

    uint16_t subBlockCountForIndex(uint16_t groupIndex) const {
        assert(groupIndex < Max && "Index out of range.");
        return _nSubBlocks[groupIndex];
    }

    bool isFree(uint16_t groupIndex, uint16_t subBlockIndex) const {
        assert(groupIndex < Max && "Index out of range.");
        byte_t * freeList = _map[groupIndex];
        assert(data() <= freeList && freeList < data() + _dataSize &&
            "Free list not in expected range. Did FSA move memory location since init? "
            "Be sure to call updateMap() if that happens.");
        return isFree(freeList, subBlockIndex);
    }

    byte_t const * data() const {
        return (byte_t const *)alignPtr((byte_t *)this + sizeof(FSA), _align);
    }

    // INTERNALS
private:
    FSA(Setup const & setup) {
        _align = setup.align;
        _dataSize = DataSize(setup);
        for (uint16_t i = 0; i < Max; ++i) {
            _nSubBlocks[i] = setup.nSubBlocks[i];
        }
        updateMap();
        printInfo();
    }

    void updateMap() {
        byte_t * nextPtr = (byte_t *)data();
        for (size_t i = 0; i < Max; ++i) {
            if (_nSubBlocks[i] == 0) {
                _map[i] = nullptr;
                continue;
            }
            _map[i] = nextPtr;
            nextPtr +=
                FreeListByteSize(_nSubBlocks[i], _align) +
                alignSize(SubBlockByteSize(i) * _nSubBlocks[i], _align);
        }
        assert(_dataSize == nextPtr - data() && "Unexpected data size.");
    }

    byte_t * data() {
        return (byte_t *)alignPtr((byte_t *)this + sizeof(FSA), _align);
    }

    bool isFree(byte_t * freeList, uint16_t index) const {
        return ((freeList[index/8]) >> (index % 8)) & 1;
    }

    void setClaimed(byte_t * freeList, uint16_t index) {
        freeList[index/8] |= (uint8_t)(index % 8);
    }

    void setFree(byte_t * freeList, uint16_t index) {
        freeList[index/8] &= ((uint8_t)(index % 8) ^ 0xff);
    }

    void printInfo() {
        printl("FSA base: %p", this);
        printl("FSA data: %p", data());
        printl("sizeof(FSA): %zu", sizeof(FSA));
        for (size_t i = 0; i < Max; ++i) {
            if (_nSubBlocks[i] == 0) {
                continue;
            }
            printl("--------------");
            printl("_nSubBlocks[%zu] = %d", i, _nSubBlocks[i]);
            printl("_map[%zu] = %p", i, _map[i]);
            printl("FreeListByteSize %zu", FreeListByteSize(_nSubBlocks[i], _align));
            printl("SubBlocksByteSize %zu", alignSize(SubBlockByteSize(i) * _nSubBlocks[i], _align));
        }
        printl("dataSize = %zu", _dataSize);
    }

    // STORAGE
private:
    // pointers to subblock groups. _map[0] is pointer to 2-byte subblock group, etc.
    // nullptr means group of that byte size does not exist.
    byte_t * _map[Max] = {nullptr};
    // subblock counts. _nSubBlocks[0] is number of 2-byte subblocks, etc.
    uint16_t _nSubBlocks[Max] = {0};
    // total size in bytes
    size_t _dataSize = 0;
    // alignment
    size_t _align = 0;

    // STATIC INTERNALS
private:
    static uint16_t SubBlockByteSize(uint8_t index) {
        assert(index < Max && "Index out of range.");
        return (uint16_t)1 << (index+1);
    }

    static uint16_t FreeListByteSize(uint16_t nSubBlocks, size_t align = 0) {
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
//     friend class MemMan2;

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
