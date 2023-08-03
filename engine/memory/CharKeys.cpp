#include "CharKeys.h"
#include <new>
// #include "../dev/print.h"

void CharKeys::Node::setKey(char const * key) {
    for (size_t i = 0; i < KEY_MAX; ++i) {
        this->key[i] = key[i];
        if (key[i] == '\0') {
            // printl("broke early");
            return;
        }
    }
    // printl("setting key[%zu] to null (was %c)", KEY_MAX-1, this->key[KEY_MAX-1]);
    this->key[KEY_MAX-1] = '\0';
}

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

CharKeys::Status CharKeys::add(char const * key, void * ptr) {
    return treeInsert(_root, key, ptr);
}

bool CharKeys::hasKey(char const * key) const {
    Node const * found = treeSearch(_root, key);
    return (found != nullptr);
}

void * CharKeys::operator[](char const * key) const {
    Node const * found = treeSearch(_root, key);
    return (found) ? found->ptr : nullptr;
}

void * CharKeys::ptrForKey(char const * key) const {
    Node const * found = treeSearch(_root, key);
    return (found) ? found->ptr : nullptr;
}

CharKeys::Node * CharKeys::nodeForKey(char const * key) {
    return (Node *)treeSearch(_root, key);
}

CharKeys::Node const * CharKeys::nodeForKey(char const * key) const {
    return treeSearch(_root, key);
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
        node->setKey(key);
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

CharKeys::Node const * CharKeys::treeSearch(Node * node, char const * key) const {
    // tree/branch empty
    if (node == nullptr) {
        return nullptr;
    }
    // compare strings
    int compare = strcmp(key, node->key);
    // key == node->key; found it!
    if (compare == 0) {
        return node;
    }
    // key < node->key
    else if (compare < 0) {
        return treeSearch(node->left, key);
    }
    // key > node->key
    else {
        return treeSearch(node->right, key);
    }
}

size_t CharKeys::nNodes() const {
    return pool()->_nClaimed;
}

bool CharKeys::isFull() const {
    return pool()->isFull();
}

