#include "FSA.h"
#include <string.h>
#include "../dev/print.h"


uint16_t FSA::subBlockCountForGroup(uint16_t groupIndex) const {
    assert(groupIndex < Max && "Index out of range.");
    return _nSubBlocks[groupIndex];
}

byte_t const * FSA::freeListPtrForGroup(uint16_t groupIndex) const {
    assert(groupIndex < Max && "Index out of range.");
    return _freeList[groupIndex];
}

bool FSA::isFree(uint16_t groupIndex, uint16_t subBlockIndex) const {
    // range checks
    assert(groupIndex < Max && "Group index out of range.");
    assert(subBlockIndex < _nSubBlocks[groupIndex] && "Sub-block index out of range.");

    byte_t * freeList = _freeList[groupIndex];

    #if DEBUG
    assert(data() <= freeList && freeList < data() + _dataSize &&
        "Free-list not in expected range. Did FSA move memory location since init? "
        "Be sure to call updateMap() if that happens.");
    #endif // DEBUG

    // LSB it first sub-index to check, MSB is last
    bool bitValue = (freeList[subBlockIndex/8] >> (subBlockIndex % 8)) & 1;
    return (bitValue == false);
}

bool FSA::indicesForPtr(void * ptr, uint16_t * foundGroupIndex, uint16_t * foundSubBlockIndex) const {
    // bail if pointer outside our FSA data block
    if (ptr <= data() || ptr >= data() + _dataSize) {
        return false;
    }

    byte_t * bptr = (byte_t *)ptr;
    uint16_t groupIndex = 0;
    uint16_t byteSize = 0;
    uint16_t subBlockIndex = 0;
    for (; groupIndex < Max; ++groupIndex) {
        // this group doesn't exist
        if (_groupBase[groupIndex] == nullptr) {
            continue;
        }

        // we have gone past the bptr. it must not be valid!
        if (bptr < _groupBase[groupIndex]) {
            return false;
        }

        // subBlockIndex will be calculated, but too big if we haven't gotten
        // to the real group yet.
        // for example if our pointer is the 4-byte block, the index will
        // always be out of range when considering the 2-byte block.
        byteSize = 2 << groupIndex;
        subBlockIndex = (bptr - _groupBase[groupIndex]) / byteSize;

        // we have a match!
        if (subBlockIndex < _nSubBlocks[groupIndex]) {
            if (foundGroupIndex) *foundGroupIndex = groupIndex;
            if (foundSubBlockIndex) *foundSubBlockIndex = subBlockIndex;
            return true;
        }
    }

    return false;
}


byte_t const * FSA::data() const {
    return (byte_t const *)alignPtr((byte_t *)this + sizeof(FSA), _align);
}

FSA::FSA(Setup const & setup) {
    _align = setup.align;
    _dataSize = DataSize(setup);

    // set sub-block counts for all groups
    for (uint16_t i = 0; i < Max; ++i) {
        _nSubBlocks[i] = setup.nSubBlocks[i];
    }

    updateMap();
    setAllFree();

    printInfo();
}

void FSA::updateMap() {
    byte_t * nextPtr = (byte_t *)data();
    for (size_t i = 0; i < Max; ++i) {
        if (_nSubBlocks[i] == 0) {
            _freeList[i] = nullptr;
            _groupBase[i] = nullptr;
            continue;
        }
        _freeList[i] = nextPtr;
        uint16_t freeListByteSize = FreeListByteSize(_nSubBlocks[i], _align);
        _groupBase[i] = nextPtr + freeListByteSize;
        nextPtr +=
            freeListByteSize +
            alignSize(SubBlockByteSize(i) * _nSubBlocks[i], _align);
    }
    assert(_dataSize == nextPtr - data() && "Unexpected data size.");
}

byte_t * FSA::data() {
    return (byte_t *)alignPtr((byte_t *)this + sizeof(FSA), _align);
}

void * FSA::alloc(size_t size) {
    // quick exit
    if (size > MaxBytes) return nullptr;
    if (size < MinBytes) return nullptr;

    // find index of appropriate group (smallest sub-block size that fits size)
    uint8_t groupIndex = 0;
    uint16_t test = 2;
    while(size > test && groupIndex < Max) {
        test <<= 1;
        groupIndex += 1;
    }

    // size was too big
    if (groupIndex >= Max) return nullptr;

    // actual byte size of free-list without trailing padding
    // this also ensures nSubBlocks for this group is multiple of 8.
    uint16_t freeListByteSize = FreeListByteSize(_nSubBlocks[groupIndex], 0);

    // loop through free-list bytes
    for (uint16_t freeListByte = 0; freeListByte < freeListByteSize; ++freeListByte) {
        // ensures there is a free bit to find in this byte
        if (_freeList[groupIndex][freeListByte] == 0xff) {
            continue;
        }

        // LSB it first sub-index to check, MSB is last
        uint8_t bitIndex = 0;
        uint8_t byte = _freeList[groupIndex][freeListByte];
        while (byte & 1) {
            byte >>= 1;
            bitIndex += 1;

            // TODO: take this check out, even in DEBUG mode.
            // only here to ward against coding errors.
            #if DEBUG
            assert(bitIndex < 8 && "Debug double-check we found a free bit. "
                "(Earlier test against 0xff should have solved");
            #endif // DEBUG
        }

        uint16_t subBlockIndex = freeListByte * 8 + bitIndex;

        assert(subBlockIndex < _nSubBlocks[groupIndex] && "Sub-block index out of range.");

        void * ptr = (void *)subBlockPtr(groupIndex, subBlockIndex);
        setClaimed(_freeList[groupIndex], subBlockIndex);
        return ptr;
    }
    return nullptr;
}

bool FSA::destroy(void * ptr) {
    // bail if pointer outside our FSA data block
    if (ptr <= data() || ptr >= data() + _dataSize) {
        return false;
    }

    byte_t * bptr = (byte_t *)ptr;
    uint16_t groupIndex = 0;
    uint16_t byteSize = 0;
    uint16_t subBlockIndex = 0;
    for (; groupIndex < Max; ++groupIndex) {
        // this group doesn't exist
        if (_groupBase[groupIndex] == nullptr) {
            continue;
        }

        // we have gone past the bptr. it must not be valid!
        if (bptr < _groupBase[groupIndex]) {
            return false;
        }

        // subBlockIndex will be calculated, but too big if we haven't gotten
        // to the real group yet.
        // for example if our pointer is the 4-byte block, the index will
        // always be out of range when considering the 2-byte block.
        byteSize = 2 << groupIndex;
        subBlockIndex = (bptr - _groupBase[groupIndex]) / byteSize;

        // we have a match!
        if (subBlockIndex < _nSubBlocks[groupIndex]) {
            setFree(_freeList[groupIndex], subBlockIndex);
            memset(bptr, 0x00, byteSize);
            return true;
        }
    }

    return false;
}

byte_t * FSA::subBlockPtr(uint16_t groupIndex, uint16_t subBlockIndex) {
    assert(groupIndex < Max && "Group index out of range.");
    assert(subBlockIndex < _nSubBlocks[groupIndex] && "Sub-block index out of range");

    return
        _groupBase[groupIndex] +            // base of subblocks
        (2 << groupIndex) * subBlockIndex;  // sub-block byte-size * sub-block index
}

void FSA::setClaimed(byte_t * freeList, uint16_t subBlockIndex) {
    freeList[subBlockIndex/8] |= (uint8_t)1 << (subBlockIndex % 8);
}

void FSA::setFree(byte_t * freeList, uint16_t subBlockIndex) {
    freeList[subBlockIndex/8] &= ((uint8_t)1 << (subBlockIndex % 8)) ^ 0xff;
}

void FSA::setAllFree() {
    for (uint16_t groupIndex = 0; groupIndex < Max; ++groupIndex) {
        setAllFreeInGroup(groupIndex);
    }
}

void FSA::setAllFreeInGroup(uint16_t groupIndex) {
    // printl("setAllFreeInGroup(%d)", groupIndex);

    assert(groupIndex < Max && "Index out of range.");

    // bail early if no sub-blocks in this group
    if (_nSubBlocks[groupIndex] == 0) return;

    byte_t * freeList = _freeList[groupIndex];

    #if DEBUG
    assert(data() <= freeList && freeList < data() + _dataSize &&
        "Free-list not in expected range. Did FSA move memory location since init? "
        "Be sure to call updateMap() if that happens.");
    #endif // DEBUG

    uint16_t byteSize = FreeListByteSize(_nSubBlocks[groupIndex], _align);
    // printl("_nSubBlocks[%d]=%d, free-list byte size: %d, writing to: %p",
    //     groupIndex, _nSubBlocks[groupIndex], byteSize, freeList);
    memset(freeList, 0x00, byteSize);
}

void FSA::printInfo() {
    printl("FSA base: %p", this);
    printl("FSA data: %p", data());
    printl("sizeof(FSA): %zu", sizeof(FSA));
    for (size_t i = 0; i < Max; ++i) {
        if (_nSubBlocks[i] == 0) {
            continue;
        }
        printl("--------------");
        printl("_nSubBlocks[%zu] = %d", i, _nSubBlocks[i]);
        printl("_freeList[%zu] = %p", i, _freeList[i]);
        printl("_groupBase[%zu] = %p", i, _groupBase[i]);
        printl("FreeListByteSize %zu", FreeListByteSize(_nSubBlocks[i], _align));
        printl("SubBlocksByteSize %zu", alignSize(SubBlockByteSize(i) * _nSubBlocks[i], _align));
    }
    printl("dataSize = %zu", _dataSize);
}
