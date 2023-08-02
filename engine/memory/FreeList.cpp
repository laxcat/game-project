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
    assert(index < _size && "Out of range.");
    // LSB is first sub-index to check, MSB is last
    return (data()[index/8] >> (index % 8)) & 1;
}

FreeList::FreeList(size_t maxSize) :
    _size(maxSize),
    _nSlots(ItemCount(maxSize)) {
    reset();
}

bool FreeList::claim(size_t * foundIndex) {
    *foundIndex = _firstFree;
    if (isFull()) {
        return false;
    }
    size_t byteIndex = _firstFree / 8;
    size_t bitIndex =  _firstFree % 8;
    byte_t mask = (byte_t)1 << bitIndex;
    data()[byteIndex] |= mask;
    _firstFree = findFirstFree(_firstFree + 1);
    return true;
}

void FreeList::release(size_t index) {
    assert(index < _size && "Out of range.");
    size_t byteIndex = index / 8;
    size_t bitIndex =  index % 8;
    byte_t mask = ((byte_t)1 << bitIndex) ^ 0xff;
    data()[byteIndex] &= mask;
    if (_firstFree > index) {
        _firstFree = index;
    }
}

void FreeList::claimAll() {
    size_t nChunks = _nSlots / 64;
    for (size_t i = 0; i < nChunks; ++i) {
        dataChunks()[i] = UINT64_MAX;
    }
    _firstFree = _size; // make sure this matches isFull()
}

void FreeList::reset() {
    size_t nChunks = _nSlots / 64;
    for (size_t i = 0; i < nChunks; ++i) {
        dataChunks()[i] = 0;
    }
    _firstFree = 0;
}

bool FreeList::isFull() const {
    return (_firstFree == _size);
}

bool FreeList::isFree(size_t index) const {
    if (index >= _size) {
        return false;
    }
    return ((*this)[index] == false);
}

size_t FreeList::nSlots() const { return _nSlots; }
size_t FreeList::size() const { return _size; }

size_t FreeList::findFirstFree(size_t start) {
    size_t nChunks = _nSlots / 64;
    size_t chunkIndex = start / 64;
    size_t byteIndex = 0;
    size_t bitIndex = 0;

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

        // the real index, which might be beyond size
        size_t index = (chunkIndex * 64) + (byteIndex * 8) + bitIndex;
        // never return greater than size
        return (index > _size) ? _size : index;
    }

    // not found
    return _size;
}
