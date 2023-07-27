// #pragma once
// #include <stdarg.h>
// #include <stdio.h>

// /*
// Container that assumes memory in _ptr to be initalized, fixed size. no head.

// Designed to be used within pre-allocated memory, like inside a MemMan::Block.
// Expects `_size` bytes of pre-allocated (safe) memory directly after its own instance.
// */

// template <typename T>
// class View {
// public:
//     friend class MemMan;
    
//     View() : _ptr(nullptr), _count(0) {}
//     View(T * ptr, size_t count) : _ptr(ptr), _count(count) {}
//     size_t count() const { return _count; }
//     bool isValid() const { return _ptr != nullptr && _count > 0; }
//     T & operator[](size_t i) { assert(_ptr && i < _count); return _ptr[i]; }
//     T const & operator[](size_t i) const { assert(_ptr && i < _count); return _ptr[i]; }

// private:
//     T * _ptr;
//     size_t _count;
// };
