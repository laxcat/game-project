class Stack {
public:
    friend class MemSys;

    size_t size() const { return _size; }
    size_t head() const { return _head; }
    byte_t * data() const { return (byte_t *)this + sizeof(Stack); }
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
        char * str = (char *)dataHead();
        va_list args;
        va_start(args, fmt);
        int written = vsnprintf(str, _size - _head, fmt, args);
        va_end(args);

        // vsnprintf writes '\0' to buffer but doesn't count it in return value
        if (written) _head += written + 1;

        if (_head > _size) _head = _size;
        return str;
    }

    void reset() { _head = 0; }

private:
    size_t _size = 0;
    size_t _head = 0;

    Stack(size_t size) :
        _size(size)
    {
    }
};
