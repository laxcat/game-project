// #pragma once
// #include "../common/types.h"

// !!!!!!!!! UNUSED !!!!!!!!!

// /*
//  Designed to be used in MemMan::Block. Actual storage is directly after this
//  class's location in memory.
// */

// class BitArray {
// public:

//     static constexpr size_t ByteSizeForSize(uint16_t size) {
//         assert(size / 8 * 8 == size && "size must be multiple of 8.");
//         return sizeof(BitArray) + size / 8;
//     }

//     BitArray(uint16_t size) {
//         _size = size;
//     }

//     size_t         size() const { return _size; }
//     byte_t       * data()       { return (byte_t *)this + sizeof(*this); }
//     byte_t const * data() const { return (byte_t const *)((byte_t *)this + sizeof(*this)); }
// private:
//     uint16_t _size;
// };

