#pragma once
#include "BitArray.h"

/*
Map designed to lookup FSA locations in memory given requested byte size.

Supports 12 diffent FSA blocks from of subblock sizes 2^1 (2) to 2^12 (4096).
*/
class FSAMap {
public:

    static int constexpr MinBytes = 1;    // smallest block is 2^1, 1 byte request uses 2-byte blocks
    static int constexpr MaxBytes = 4096; // 2^MaxBlocks
    static int constexpr MaxBlocks = 12;

    byte_t * activePtrForByteSize(uint16_t byteSize) {
        uint8_t ideal = IndexForByteSize(byteSize);
        while (ideal < MaxBlocks && _ptrs[ideal] == nullptr) {
            ++ideal;
        }
        if (ideal == MaxBlocks) return nullptr;
        return _ptrs[ideal];

    }

    byte_t * ptrForByteSize(uint16_t byteSize) {
        return _ptrs[IndexForByteSize(byteSize)];
    }

    void setPtrForByteSize(uint16_t byteSize, byte_t * ptr) {
        _ptrs[IndexForByteSize(byteSize)] = ptr;
    }

    bool hasPtrForByteSize(uint16_t byteSize) {
        return (_ptrs[IndexForByteSize(byteSize)] != nullptr);
    }

    static size_t constexpr ActualByteSizeForByteSize(uint16_t byteSize) {
        uint8_t exp = IndexForByteSize(byteSize) + 1;
        return (size_t)((uint16_t)1 << exp);
    }

private:
    byte_t * _ptrs[MaxBlocks] = {nullptr};   // pointers

    // returns 0-11, given byteSize range MinBytes-MaxBytes
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


class FSA {
public:
    static size_t constexpr BlockByteSize(uint8_t subBlockByteSize, uint16_t subBlockCount) {
        assert(subBlockCount / 8 * 8 == subBlockCount && "subBlockCount must be multiple of 8.");
        return
            sizeof(FSA) +
            BitArray::ByteSizeForSize(subBlockCount) +
            FSAMap::ActualByteSizeForByteSize(subBlockByteSize) * subBlockCount;
    }

    FSA(uint8_t subBlockByteSize, uint16_t subBlockCount) {
        assert(subBlockCount / 8 * 8 == subBlockCount && "subBlockCount must be multiple of 8.");
    }

private:
    uint8_t _subBlockSize = 0;
    uint16_t _nSubBlocks = 0;
};


