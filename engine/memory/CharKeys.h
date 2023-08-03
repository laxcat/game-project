#pragma once
#include <stdio.h>
#include <string.h>
#include "Pool.h"

/*
Binary tree that uses a short char string for the key (used to sort nodes),
with a void* on each node.
Designed to be used in pre-allocated memory.

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
        Node * left = nullptr;
        Node * right = nullptr;
    };

    enum Status {
        SUCCESS = 0,
        DUPLICATE_KEY,
        BUFFER_FULL
    };

    using PoolT = Pool<Node>;

    // API
public:
    CharKeys(size_t size);

    Status add(char const * key, void * ptr);

    size_t nNodes() const;
    bool isFull() const;

    // STORAGE
private:
    size_t _size;
    Node * _root = nullptr;

    // INTERNALS
private:
    byte_t       * data()       ;
    byte_t const * data()  const;
    PoolT        * pool()       ;
    PoolT  const * pool()  const;
    Node         * nodes()      ;
    Node   const * nodes() const;

    Status treeInsert(Node *& root, char const * key, void * ptr);

    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
