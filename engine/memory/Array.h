#pragma once
#include <functional>
#include <assert.h>
#include <stddef.h>
#include "../common/types.h"
#include "../common/debug_defines.h"

/*
Array
- Requires setting max available buffer size up front
- Bounds checking on subscript access (unchecked on data() access)
- Designed to be used within pre-allocated memory, like inside a MemMan Block.
    Expects sizeof(T) * _maxSize bytes of pre-allocated (safe) memory directly
    after its own instance.

*/

template <typename T>
class Array {
// TYPES
public:
    using FindFn = std::function<bool(T const &)>;

// INIT
private:
    friend class MemMan;
    Array(size_t maxSize) : _maxSize(maxSize) {}

// STATIC INTERFACE
public:
    static constexpr size_t DataSize(size_t max) {
        return max * sizeof(T);
    }

// INTERFACE
public:
    T & insert(size_t i, T const & item) {
        assert(i <= _size && "Out of range.");
        assert(_size < _maxSize && "Cannot insert into Array, _size == _maxSize.");
        copy(i + 1, i, _size - i);
        ++_size;
        return data()[i] = item;
    }

    T & append(T const & item) {
        assert(_size < _maxSize && "Cannot insert into Array, _size == _maxSize.");
        return data()[_size++] = item;
    }

    void remove(size_t i, size_t count = 1) {
        assert(i + count <= _size && "Out of range.");
        copy(i, i + count, _size - i - count);
        _size -= count;

        #if DEBUG
        for (size_t i = _size; i < _size + count; ++i) {
            data()[i] = {};
        }
        #endif // DEBUG
    }

    T const * find(FindFn const & comparison) const {
        for (size_t i = 0; i < _size; ++i) {
            if (comparison(data()[i])) {
                return data() + i;
            }
        }
        return nullptr;
    }

    size_t size() const { return _size; }
    size_t maxSize() const { return _maxSize; }

    T       * data()       { return (T       *)((byte_t *)this + sizeof(Array)); }
    T const * data() const { return (T const *)((byte_t *)this + sizeof(Array)); }

    T       & operator[](size_t i)       { assert(i < _size && "Out of range."); return data()[i]; }
    T const & operator[](size_t i) const { assert(i < _size && "Out of range."); return data()[i]; }

    bool bufferFull() const { return _size == _maxSize; }

// STORAGE
private:
    size_t _size = 0;
    size_t _maxSize;

    // copies count items from src to dst.
    // for internal use only; calling fn should modify _size to appropriate value afterwards.
    // will copy items to beyond last item.
    // won't copy src from beyond last item, and obviously won't access beyond buffer.
    // will return actual number of items copied
    size_t copy(size_t dst, size_t src, size_t count) {
        // nothing to copy
        if (dst == src || count == 0) {
            return count;
        }
        // fail silently when entirely out of bounds
        if (src >= _size || dst >= _maxSize) {
            return 0;
        }

        // calc actual number of items to copy
        // new count will be smallest number to copy without going out of bounds
        // or copying src items beyond last item
        if (src + count > _size) {
            count = _size - src;
        }
        if (dst + count > _maxSize && _maxSize - dst < count) {
            count = _maxSize - dst;
        }

        // being mindful of copy direction to avoid overwriting...
        // copying from left towards right (start at last item and work down)
        if (src < dst) {
            for (size_t i = 0; i < count; ++i) {
                data()[dst+count-1-i] = data()[src+count-1-i];
            }
        }
        // copying from right towards left (start at first item and work up)
        else {
            for (size_t i = 0; i < count; ++i) {
                data()[dst+i] = data()[src+i];
            }
        }
        return count;
    }

// DEBUG / DEV INTERFACE
private:
    #if DEV_INTERFACE
    friend void Array_editorCreate();
    friend void Array_editorEditBlock(Array<int> &);
    #endif // DEV_INTERFACE
};

#if DEV_INTERFACE
void Array_editorCreate();
void Array_editorEditBlock(Array<int> & arr);
#endif // DEV_INTERFACE
