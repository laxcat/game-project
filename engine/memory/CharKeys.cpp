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
    Node const * found = search(key);
    return (found != nullptr);
}

void * CharKeys::operator[](char const * key) const {
    Node const * found = search(key);
    return (found) ? found->ptr : nullptr;
}

void * CharKeys::ptrForKey(char const * key) const {
    Node const * found = search(key);
    return (found) ? found->ptr : nullptr;
}

size_t CharKeys::nNodes() const {
    return pool()->_nClaimed;
}

bool CharKeys::isFull() const {
    return pool()->isFull();
}

CharKeys::Node * CharKeys::nodeForKey(char const * key) {
    return search(key);
}

CharKeys::Node const * CharKeys::nodeForKey(char const * key) const {
    return search(key);
}

// some debate as to whether the casting to and from const is a good idea
// (seems like it's the best solution to DRY, and pretty safe)
// https://stackoverflow.com/questions/856542
// https://stackoverflow.com/questions/123758
CharKeys::Node * CharKeys::search(char const * key) {
    return (Node *)((CharKeys const *)this)->search(key);
}
CharKeys::Node const * CharKeys::search(char const * key) const {
    Node * node = _root;
    while(node) {
        int compare = strcmp(key, node->key);
        if      (compare == 0)  { return node; }
        else if (compare <  0)  { node = node->left; }
        else                    { node = node->right; }
    }
    return nullptr;
}

bool CharKeys::remove(char const * key) {
    Node * d = search(key);
    if (d == nullptr) {
        return false;
    }
    remove(d);
    return true;
}

void CharKeys::remove(Node * d) {
    // remove from tree
    if (d->left == nullptr) {
        shift(d, d->right);
    }
    else if (d->right == nullptr) {
        shift(d, d->left);
    }
    else {
        Node * e = successor(d);
        if (e->parent != d) {
            shift(e, e->right);
            e->right = d->right;
            e->right->parent = e;
        }
        shift(d, e);
        e->left = d->left;
        e->left->parent = e;
    }
    // remove from pool
    pool()->release(d);
}

void CharKeys::shift(Node * a, Node * b) {
    if (a->parent == nullptr) {
        _root = b;
    }
    else if (a == a->parent->left) {
        a->parent->left = b;
    }
    else {
        a->parent->right = b;
    }
    if (b != nullptr) {
        b->parent = a->parent;
    }
}

CharKeys::Node * CharKeys::successor(Node * x) const {
    if (x->right != nullptr) {
        return minimum(x->right);
    }
    Node * y = x->parent;
    while(y != nullptr && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}

CharKeys::Node * CharKeys::minimum(Node * n) const {
    while (n->left != nullptr) {
        n = n->left;
    }
    return n;
}
