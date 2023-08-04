#pragma once
#include <stdio.h>
#include <string.h>
#include "Pool.h"

/*
Binary tree that uses a short char string for the key (used to sort nodes),
with a void* on each node.
Designed to be used in pre-allocated memory.

FEATURES:
    - Forward iterator
    - Extra "next" pointer in node for constant-time forward iteration
TODO:
    - Red-black impl

Memory layout:

|------------|-----------------...---|
 CharKeys    Pool<Node>
             ^
             pool()
             |---- DataSize ---...---|

*/

class CharKeys {
    // FRINDS / STATICS
public:
    friend class MemMan;

    static constexpr size_t KEY_MAX = 16;

    static constexpr size_t DataSize(size_t size) {
        return sizeof(PoolT) + PoolT::DataSize(size);
    }

    // SUB-TYPES
public:
    class Node {
    public:
        char key[KEY_MAX];
        void * ptr = nullptr;
        Node * parent = nullptr;
        Node * left = nullptr;
        Node * right = nullptr;
        Node * next = nullptr; // set to successor(this)

        void setKey(char const * key); // might as well avoid snprintf since it's easy
        void * operator*() const { return ptr; }
    };

    enum Status {
        SUCCESS = 0,
        DUPLICATE_KEY,
        BUFFER_FULL
    };

    using PoolT = Pool<Node>;

    // forward iterator
    struct Iterator {
        Iterator(Node * node);
        Iterator operator++();
        bool operator!=(Iterator const & other) const;
        Node const * operator*() const;
    private:
        Node * _node;
    };

    // API
public:
    // init
    CharKeys(size_t size);

    // modify tree
    Status insert(char const * key, void * ptr);
    bool remove(char const * key);

    // access/query
    bool hasKey(char const * key) const;
    void * operator[](char const * key) const;
    void * ptrForKey(char const * key) const;
    size_t nNodes() const;
    bool isFull() const;

    // iteration (inorder by key)
    Iterator begin() const;
    Iterator end() const;

    // STORAGE
private:
    size_t _size;
    Node * _root = nullptr;
    Node * _first = nullptr; // cache "first" (inorder) node

    // INTERNALS
private:
    // data access
    byte_t       * data()       ;
    byte_t const * data()  const;
    PoolT        * pool()       ;
    PoolT  const * pool()  const;
    Node         * nodes()      ;
    Node   const * nodes() const;

    // search
    Node       * nodeForKey(char const * key);
    Node const * nodeForKey(char const * key) const;
    Node       * search(char const * key);
    Node const * search(char const * key) const;

    // modify
    void remove(Node * d);

    // helper
    void shift(Node * a, Node * b);
    Node * successor(Node * n) const;
    Node * predecessor(Node * n) const;
    Node * maximum(Node * n) const;
    Node * minimum(Node * n) const;

    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
