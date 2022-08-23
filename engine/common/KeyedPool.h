#pragma once
#include <iterator> // For std::forward_iterator_tag
#include <cstddef>  // For std::ptrdiff_t
#include <unordered_map>
#include <sstream>
#include "../dev/print.h"


// Iterable container
// Inserted objects keep memory location for lifetime.
// Objects inserted sequentialy, but new objects will use old free slots if
// objects have been erased, so should be only used if object order is 
// unimportant.
// In other words, iteration happens on memory order, not insertion order.

template <typename K, typename T, size_t MAX=16>
class KeyedPool {
public:
    // forward iterator
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T *;
        using reference         = T &;
        using container         = KeyedPool *;

        Iterator(container c, pointer ptr) : _container(c), _ptr(ptr) {}

        reference operator*() const { return *_ptr; }
        pointer operator->() { return _ptr; }
        Iterator& operator++() {
            _ptr = _container->_pool + _container->nextAfter(_ptr - _container->_pool);
            return *this;
        }
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
        bool operator==(Iterator const & other) { return _ptr == other._ptr; };
        bool operator!=(Iterator const & other) { return _ptr != other._ptr; };
        difference_type operator-(Iterator const & other) {
            if (_container != other._container) {
                throw std::logic_error{"Invalid, containers must match."};
            }
            return _ptr - other._ptr;
        }

    private:
        container _container;
        pointer _ptr;
    };

    KeyedPool() {
        for (size_t i = 0; i < MAX; ++i) _free[i] = true;
    }

    size_t size() const { return _size; }
    Iterator begin() { return Iterator(this, &_pool[_begin]); }
    Iterator end()   { return Iterator(this, &_pool[_end]); }

    Iterator create(K const & key) {
        // if key exists do nothing
        if (_keys.count(key)) {
            // IS this a warning?
            // print("WARNING: ignoring create. Key already exists.");
            return Iterator(this, &_pool[_keys[key]]);
        }

        // no create
        if (_size == MAX) {
            print("WARNING: ignoring create. Pool full.");
            return end();
        }

        size_t index = _firstFree;
        ++_size;
        if (_end == index) ++_end;
        if (_begin > index) _begin = index;
        _keys[key] = index;
        _free[index] = false;
        _firstFree = nextFreeAfter(index);
        return Iterator(this, &_pool[index]);
    }

    Iterator insert(K const & key, T const & item) {
        // if key exists already, replace object
        if (_keys.count(key)) {
            size_t index = _keys[key];
            _pool[index] = item;
            return Iterator(this, &_pool[index]);
        }

        // no insert
        if (_size == MAX) {
            print("WARNING: ignoring insert. Pool full.");
            return end();
        }

        // new insert
        auto created = create(key);
        *created = item;
        return created;
    }

    Iterator erase(K const & key) {
        if (_keys.count(key) == 0) {
            print("WARNING: ignoring erase. Key not found.");
            return end();
        }

        size_t index = _keys[key];
        _pool[index] = {};
        _free[index] = true;
        if (_firstFree > index) {
            _firstFree = index;
        }
        --_size;
        if (_size == 0) {
            _begin = 0;
            _end = 0;
        }
        else {
            if (index == _end - 1) {
                --_end;
            }
            if (index == _begin) {
                _begin = nextAfter(index);
            }
        }
        _keys.erase(key);

        return Iterator(this, &_pool[nextAfter(index)]);
    }

    T & at(K const & key) {
        return _pool[_keys.at(key)];
    }

    bool contains(K const & key) const {
        return _keys.count(key);
    }

    std::string toString() const {
        std::stringstream s;
        s << "(";
        for (size_t i = 0; i < _end; ++i) {
            s << i << ":";
            if (_free[i]) {
                s << "<free>";
            }
            else {
                auto it = find_if(_keys.begin(), _keys.end(), [i](auto const & a){
                    return a.second == i;
                });
                if (it == _keys.end()) {
                    s << "<ERROR: NO KEY FOUND>";
                }
                else {
                    s << it->first;
                }
            }
            if (i < _end - 1) {
                s << ", ";
            }
        }

        // debug
        if constexpr (false) {
            s << 
            " (b" << _begin << 
            ",e" << _end << 
            ",ff" << _firstFree << 
            ",s" << _size << ")";
        }

        s << ")";
        return s.str();
    }

private:
    T _pool[MAX];
    bool _free[MAX];
    std::unordered_map<K, size_t> _keys;
    size_t _begin = 0;
    size_t _end = 0;
    size_t _firstFree = 0;
    size_t _size = 0;

    size_t nextFreeAfter(size_t index) const {
        do ++index; while(!_free[index] && index < _end);
        return index;
    }

    size_t nextAfter(size_t index) const {
        if (index >= _end) return _end;
        do ++index; while(_free[index] && index < _end);
        return index;
    }
};
