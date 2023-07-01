#pragma once

/*
Container that assumed contents are uninitialized. Has reserved space, but also has head (count)

Designed to be used within pre-allocated memory, like inside a MemMan Block.
Expects sizeof(T) * _maxSize bytes of pre-allocated (safe) memory directly after its own instance.
*/

template <typename T>
class Array {
public:
    static constexpr size_t DataSize(size_t max) {
        return max * sizeof(T);
    }

    Array(size_t maxSize) : _maxSize(maxSize) {}

    T & insert(size_t i, T const & item) {
        shiftItems(i, 1);
        data()[i] = item;
    }

    size_t size() const { return _size; }
    size_t maxSize() const { return _maxSize; }

    T       * data()       { return (T       *)((byte_t *)this + sizeof(Array)); }
    T const * data() const { return (T const *)((byte_t *)this + sizeof(Array)); }

    T       & operator[](size_t i)       { assert(i < _size); return data()[i]; }
    T const & operator[](size_t i) const { assert(i < _size); return data()[i]; }

private:
    size_t _size = 0;
    size_t _maxSize;

    void shiftItems(size_t start, int shift, size_t count = 0) {
        if (count == 0) {
            count = _size - start;
        }
        assert(start + count <= _maxSize && "Not enough allocated item slots to shift.");

        if (shift == 0) return;

        // shifting left
        if (shift < 0) {
            assert(start >= -shift);
            int end = start + count;
            for (int i = start; i < end; ++i) {
                data()[i + shift] = data()[i];
            }
        }
        // shifting right
        else {
            int end = start + count;
            assert(end <= _size - shift);
            for (int i = end - 1; i >= start; --i) {
                data()[i + shift] = data()[i];
            }
        }

        _size += shift;
    }
};
