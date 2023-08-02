#include "CharKeys.h"

CharKeys::CharKeys(size_t size) :
    _size(size) {
}

byte_t * CharKeys::data() { return (byte_t *)this + sizeof(CharKeys); }
CharKeys::PoolT * CharKeys::pool() { return (PoolT *)data(); }
CharKeys::Node * CharKeys::nodes() { return pool()->dataItems(); }
