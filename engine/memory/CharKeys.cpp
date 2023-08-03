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

CharKeys::Status CharKeys::insert(char const * key, void * ptr) {
    // if pool full
    if (pool()->isFull()) {
        return BUFFER_FULL;
    }
    // find insersion point
    Node * parent = nullptr;
    Node * runner = _root;
    int compare;
    while (runner) {
        parent = runner;
        compare = strcmp(key, parent->key);
        if (compare == 0) {
            return DUPLICATE_KEY;
        }
        else if (compare < 0) {
            runner = runner->left;
        }
        else {
            runner = runner->right;
        }
    }
    // create node in buffer
    Node * newNode = pool()->claim();
    newNode->setKey(key);
    newNode->ptr = ptr;
    newNode->parent = parent;
    // connect newNode to tree
    if (parent == nullptr) {
        _root = newNode;
        return SUCCESS;
    }
    // compare will still hold comparison of (key < parent->key)
    // key < parent->key
    if (compare < 0) { parent->left = newNode; }
    // key >= parent->key
    else             { parent->right = newNode; }
    return SUCCESS;
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

