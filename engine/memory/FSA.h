#pragma once

class FSA {
public:
    static size_t constexpr TotalBlockSize(uint8_t subBlockByteSize, uint16_t subBlockCount) {
        assert(subBlockCount / 8 * 8 == subBlockCount && "subBlockCount must be multiple of 8.");
        return
            sizeof(FSA) +
            subBlockCount / 8 +
            subBlockByteSize * subBlockCount;
    }

    FSA(uint8_t subBlockByteSize, uint16_t subBlockCount) {
        assert(subBlockCount / 8 * 8 == subBlockCount && "subBlockCount must be multiple of 8.");
    }

private:
    uint8_t _subBlockSize = 0;
    uint16_t _nSubBlocks = 0;
};


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
        uint8_t ideal = indexForByteSize(byteSize);
        while (ideal < MaxBlocks && _ptrs[ideal] == nullptr) {
            ++ideal;
        }
        if (ideal == MaxBlocks) return nullptr;
        return _ptrs[ideal];

    }

    byte_t * ptrForByteSize(uint16_t byteSize) {
        return _ptrs[indexForByteSize(byteSize)];
    }

    void setPtrForByteSize(uint16_t byteSize, byte_t * ptr) {
        _ptrs[indexForByteSize(byteSize)] = ptr;
    }

    bool hasPtrForByteSize(uint16_t byteSize) {
        return (_ptrs[indexForByteSize(byteSize)] != nullptr);
    }

    size_t blockByteSizeForSize(uint16_t byteSize) {
        uint8_t exp = indexForByteSize(byteSize) + 1;
        return (size_t)((uint16_t)1 << exp);
    }

// private:
    byte_t * _ptrs[MaxBlocks] = {nullptr};   // pointers

    // returns 0-11, given byteSize range MinBytes-MaxBytes
    uint8_t indexForByteSize(uint16_t byteSize) {
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
    int b = map.indexForByteSize(i); // private
    int bs = map.blockByteSizeForSize(i);
    printl("indexForByteSize(%d) : %d, acutal block size: %d", i, b, bs);
}

*/
