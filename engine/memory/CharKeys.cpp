#include "CharKeys.h"
#include <stdio.h>
#include <new>

CharKeys::CharKeys(size_t size) :
    _size(size) {
    new (pool()) PoolT{size};
}

byte_t       * CharKeys::data()       { return (byte_t *)this + sizeof(CharKeys); }
byte_t const * CharKeys::data() const { return (byte_t *)this + sizeof(CharKeys); }

CharKeys::PoolT       * CharKeys::pool()       { return (PoolT *)data(); }
CharKeys::PoolT const * CharKeys::pool() const { return (PoolT *)data(); }

CharKeys::Node       * CharKeys::nodes()       { return pool()->dataItems(); }
CharKeys::Node const * CharKeys::nodes() const { return pool()->dataItems(); }

// CharKeys::Node       * CharKeys::root()       { return _root; }
// CharKeys::Node const * CharKeys::root() const { return _root; }

CharKeys::Status CharKeys::add(char const * key, void * ptr) {
    return treeInsert(_root, key, ptr);
}

CharKeys::Status CharKeys::treeInsert(Node *& node, char const * key, void * ptr) {
    // node not set
    if (node == nullptr) {
        // create new node
        node = pool()->claim();
        if (node == nullptr) {
            return BUFFER_FULL;
        }

        // copy data
        snprintf(node->key, KEY_MAX, "%s", key);
        node->ptr = ptr;

        // no problem
        return SUCCESS;
    }

    // compare strings
    int compare = strcmp(key, node->key);

    // key < node->key
    if (compare < 0) {
        return treeInsert(node->left, key, ptr);
    }

    // key > node->key
    else if (compare > 0) {
        return treeInsert(node->right, key, ptr);
    }

    // key == node->key
    return DUPLICATE_KEY;
}

size_t CharKeys::nNodes() const {
    return pool()->_nClaimed;
}

bool CharKeys::isFull() const {
    return pool()->isFull();
}

