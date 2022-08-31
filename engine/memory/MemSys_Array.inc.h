// assumed to be uninitialized. has reserved space, but also has head (count)
template <typename T>
class Array {
public:
    Array() : _ptr(nullptr), _max(0) {}
    Array(T * ptr, size_t max) : _ptr(ptr), _max(max) {}

    T * claimTo(size_t i) {
        assert(i > _count && "Index must be greated than count");
        T * r = &_ptr[_count];
        _count = i;
        return r;
    }

    T * claim(size_t i = 1) { return claimTo(_count + i); }

    // TODO: copy assignment or memcpy?
    void append(T const & obj) {
        *claim() = obj;
        // memcpy(claim(), &obj, sizeof(T));
    }

    size_t count() const { return _count; }
    size_t max() const { return _max; }
    bool isValid() const { return _ptr != nullptr; }

    T & operator[](size_t i) { assert(_ptr && i < _count); return _ptr[i]; }
    T const & operator[](size_t i) const { assert(_ptr && i < _count); return _ptr[i]; }

private:
    T * _ptr;
    size_t _max;
    size_t _count = 0;
};
