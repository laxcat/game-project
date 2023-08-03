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
        char key[KEY_MAX];
        void * ptr = nullptr;
        Node * left = nullptr;
        Node * right = nullptr;
    };

    using PoolT = Pool<Node>;

    // API
public:
    CharKeys(size_t size);

    // STORAGE
private:
    size_t _size;
    size_t _root = SIZE_MAX;

    // INTERNALS
private:
    byte_t * data();
    PoolT * pool();
    Node * nodes();

    #if DEV_INTERFACE
    static void editorCreate();
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
