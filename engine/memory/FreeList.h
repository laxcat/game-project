#pragma once
#include "../common/debug_defines.h"
#include "../common/types.h"


class FreeList {
    // FRIENDS
public:
    friend class MemMan;

    // API
public:
    static size_t constexpr ItemCount(size_t count) {
        // already a multiple of 64. return same number.
        if ((count & 0b111111) == 0) {
            return count;
        }
        // push up to next number divisible by 64
        return ((count >> 6) + 1) << 6;
    }
    static size_t constexpr DataSize(size_t max) {
        return ItemCount(max) / 8;
    }

    bool operator[](size_t index) const;
    byte_t const * data() const;

    FreeList(size_t maxSize);
    bool isFree(size_t index) const;
    bool claim(size_t * foundIndex);
    void release(size_t index);
    void claimAll();
    void reset();

    // INTERNALS
private:
    size_t _nItems; // will round up to be divisible by 64
    size_t _firstFree = 0;

    size_t findFirstFree(size_t start = 0);
    byte_t * data();
    uint64_t * dataChunks();
    uint64_t const * dataChunks() const;

    // DEV INTERFACE
private:
    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
