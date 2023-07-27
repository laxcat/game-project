#include "FreeList.h"
#include <assert.h>
#include <stdint.h>
#include "mem_utils.h"

byte_t         * FreeList::data()             { return (byte_t *)this + sizeof(FreeList); }
byte_t   const * FreeList::data()       const { return (byte_t *)this + sizeof(FreeList); }
uint64_t       * FreeList::dataChunks()       { return (uint64_t *)data(); }
uint64_t const * FreeList::dataChunks() const { return (uint64_t *)data(); }

bool FreeList::operator[](size_t index) const {
    // range check
    assert(index < _nItems && "Out of range.");
    // LSB is first sub-index to check, MSB is last
    return (data()[index/8] >> (index % 8)) & 1;
}

FreeList::FreeList(size_t nBits) {
    _nItems = ItemCount(nBits);
    reset();
}

bool FreeList::isFree(size_t index) const {
    return ((*this)[index] == false);
}

bool FreeList::claim(size_t * foundIndex) {
    *foundIndex = findFirstFree();
    if (*foundIndex == _nItems) {
        return false;
    }
    size_t byteIndex = *foundIndex / 8;
    size_t bitIndex =  *foundIndex % 8;
    byte_t mask = (byte_t)1 << bitIndex;
    data()[byteIndex] |= mask;
    return true;
}

void FreeList::release(size_t index) {
    assert(index < _nItems && "Out of range.");
    size_t byteIndex = index / 8;
    size_t bitIndex =  index % 8;
    byte_t mask = ((byte_t)1 << bitIndex) ^ 0xff;
    data()[byteIndex] &= mask;
}

void FreeList::claimAll() {
    size_t nChunks = _nItems / 64;
    for (size_t i = 0; i < nChunks; ++i) {
        dataChunks()[i] = UINT64_MAX;
    }
}

void FreeList::reset() {
    size_t nChunks = _nItems / 64;
    for (size_t i = 0; i < nChunks; ++i) {
        dataChunks()[i] = 0;
    }
}


size_t FreeList::findFirstFree() {
    size_t nChunks = _nItems / 64;
    size_t chunkIndex = 0, byteIndex = 0, bitIndex = 0;

    for (; chunkIndex < nChunks; ++chunkIndex) {
        uint64_t chunk = dataChunks()[chunkIndex];

        // chunk full (no free slots); move to next chunk
        if (chunk == UINT64_MAX) {
            continue;
        }

        // find non-full byte in chunk
        byteIndex = 0;
        while ((chunk & 0xff) == 0xff) {
            chunk >>= 8;
            ++byteIndex;
        }

        // find free bit in byte
        byte_t byte = chunk & 0xff;
        bitIndex = 0;
        while (byte & 1) {
            byte >>= 1;
            ++bitIndex;
        }

        return (chunkIndex * 64) + (byteIndex * 8) + bitIndex;
    }

    return _nItems;
}
