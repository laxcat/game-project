#pragma once
#include "../common/debug_defines.h"
#include "../common/types.h"

/*

FreeList using a bit-array and 64-bit chunks to quickly scan.
Designed to be used within pre-allocated memory.

*/

class FreeList {
// INIT
private:
    friend class MemMan;
    template<typename> friend class Pool;
    FreeList(size_t nSlots);

// PUBLIC INTERFACE
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

    bool claim(size_t * foundIndex);
    void release(size_t index);
    void claimAll();
    void reset();

    bool isFull() const;
    bool isFree(size_t index) const;
    size_t size() const;

// INTERNALS
private:
    size_t _size;   // originally requested size
    size_t _nSlots; // _size rounded up to be divisible by 64
    size_t _firstFree = 0;

    size_t findFirstFree(size_t start = 0); // returns _size it not found
    byte_t * data();
    uint64_t * dataChunks();
    size_t nSlots() const;
    uint64_t const * dataChunks() const;

// DEV INTERFACE
private:
    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
