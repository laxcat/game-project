#include "MemMan.h"
#include "mem_utils.h"

size_t MemMan::BlockInfo::paddingSize() const {
    return _padding;
}

size_t MemMan::BlockInfo::dataSize() const {
    return _dataSize;
}

size_t MemMan::BlockInfo::blockSize() const {
    return _padding + BlockInfoSize + _dataSize;
}

MemBlockType MemMan::BlockInfo::type() const {
    return _type;
}

byte_t const * MemMan::BlockInfo::data() const {
    return (byte_t *)this + BlockInfoSize;
}
byte_t * MemMan::BlockInfo::data() {
    return (byte_t *)this + BlockInfoSize;
}

byte_t const * MemMan::BlockInfo::calcUnalignedDataLoc() const {
    return data() - _padding;
}

size_t MemMan::BlockInfo::calcAlignPaddingSize(size_t align) const {
    #if DEBUG
    assert(isValid() && "Block not valid.");
    #endif // DEBUG

    // block may be aligned, but treat it as though its not to get potential
    // new align location. this is then used to calc new padding size.
    // using mem_utils function.
    return alignPadding((size_t)calcUnalignedDataLoc(), align);

    // if (align == 0) return 0;
    // size_t dataPtrAsNum = (size_t)calcUnalignedDataLoc();
    // size_t remainder = dataPtrAsNum % align;
    // return (remainder) ? align - remainder : 0;
}

size_t MemMan::BlockInfo::calcAlignDataSize(size_t align) const {
    // there may be padding already set, but this should return what the value
    // would be if re-aligned to parameter align
    return (_padding + _dataSize) - calcAlignPaddingSize(align);
}

bool MemMan::BlockInfo::isAligned(size_t align) const {
    return (_padding == calcAlignPaddingSize(align));
}

byte_t const * MemMan::BlockInfo::basePtr() const {
    return data() - BlockInfoSize - _padding;
}
byte_t * MemMan::BlockInfo::basePtr() {
    return data() - BlockInfoSize - _padding;
}

bool MemMan::BlockInfo::contains(void * ptr, size_t size) const {
    byte_t const * bptr = (byte_t const *)ptr;
    byte_t const * data = this->data();
    return (data <= bptr && bptr + size <= data + _dataSize);
}

bool MemMan::BlockInfo::isValid() const {
    return (
        #if DEBUG
        // magic string valid
        (memcmp(&BlockMagicString, _debug_magic, 4) == 0) &&
        #endif // DEBUG

        // _type valid
        ((int)_type >= 0 && (int)_type <= MEM_BLOCK_GENERIC) &&

        // if free, padding should be 0
        (_type != MEM_BLOCK_FREE || (_type == MEM_BLOCK_FREE && _padding == 0)) &&

        // expected data size
        (_dataSize < 0xffffffff) &&

        #if DEBUG
        // no unlikely pointers (indicates debug-only data overflow)
        ((size_t)_next != 0xfefefefefefefefe && (size_t)_prev != 0xfefefefefefefefe) &&
        ((size_t)_next != 0xefefefefefefefef && (size_t)_prev != 0xefefefefefefefef) &&
        ((size_t)_next != 0xffffffffffffffff && (size_t)_prev != 0xffffffffffffffff) &&
        #endif // DEBUG

        // probable padding size
        (_padding <= 256) &&

        true
    );
}

#if DEBUG
void MemMan::BlockInfo::print(size_t index) const {
    if (index == SIZE_MAX) {
        index = _debug_index;
    }
    printl("BLOCK %zu %s (%p, data: %p)", index, memBlockTypeStr(_type), this, data());
    printl("    data size: %zu", _dataSize);
    printl("    padding: %zu", _padding);
    printl("    prev: %p", _prev);
    printl("    next: %p", _next);
    printl("    debug index: %zu", _debug_index);
    printl("    magic string: %.*s", 8, _debug_magic);
}
#endif // DEBUG
