#pragma once
#include <stdarg.h>
#include <stdio.h>

/*
Designed to be used within pre-allocated memory, like inside a MemMan block.
It's like a stack, but accepts any type. Because of this it can't really "pop", just reset.
Expects `_size` bytes of pre-allocated (safe) memory directly after its own instance.
*/

class FrameStack {
public:
    friend class MemMan;

    FrameStack(size_t size) :
        _size(size)
    {
    }

    size_t size() const { return _size; }
    size_t totalSize() const { return sizeof(FrameStack) + _size; }
    size_t head() const { return _head; }
    byte_t * data() const { return (byte_t *)this + sizeof(FrameStack); }
    byte_t * dataHead() const { return data() + _head; }

    template <typename T>
    T * alloc(size_t count = 1) {
        if (_head + sizeof(T) * count > _size) return nullptr;
        T * ret = (T *)dataHead();
        _head += sizeof(T) * count;
        return ret;
    }

    byte_t * alloc(size_t size) {
        return alloc<byte_t>(size);
    }

    char * formatStr(char const * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char * str = vformatStr(fmt, args);
        va_end(args);
        return str;
    }

    char * vformatStr(char const * fmt, va_list args) {
        char * str = (char *)dataHead();
        size_t maxChars = _size - _head;
        int written = vsnprintf(str, maxChars, fmt, args);

        assert(written >= 0 && "Problem with formatStr");

        // vsnprintf writes '\0' to buffer but doesn't count it in return value
        if (written > 0) _head += written + 1;
        // vsnprintf won't have written past size, but returns number of chars it WOULD have written, so
        // head may be past. (but no memory was written there (i think))
        if (_head > _size) _head = _size;
        return str;
    }

    char * writeStr(char const * str, size_t length) {
        return formatStr("%.*s", length, str);
    }

    void reset() { _head = 0; }

private:
    size_t _size = 0;
    size_t _head = 0;
};
