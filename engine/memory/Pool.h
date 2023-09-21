#pragma once
#include <assert.h>
#include "../common/debug_defines.h"
#include "../common/types.h"
#include "FreeList.h"

#if DEBUG
#include "../dev/print.h"
#endif // DEBUG

/*
Designed to be used in pre-allocated memory.

Memory layout:

|------------|------------|------------|------------|------
 Pool         FreeList     T            T            T     ...
             ^            ^
             dataBase()   dataItems()

*/

template <typename T>
class Pool {
private:
    friend class CharKeys;
    friend class MemMan;
    Pool(size_t size) :
        _size(size) {
        new (freeList()) FreeList(size);
        for (size_t i = 0; i < _size; ++i) {
            dataItems()[i] = {};
        }
    }

public:
    static constexpr size_t DataSize(size_t size) {
        return
            sizeof(FreeList) + FreeList::DataSize(size) +
            size * sizeof(T);
    }

    T * claim(size_t * foundIndex = nullptr) {
        if (freeList()->isFull()) {
            return nullptr;
        }
        size_t index;
        bool didClaim = freeList()->claim(&index);
        if (!didClaim) {
            return nullptr;
        }
        if (foundIndex) {
            *foundIndex = index;
        }
        ++_nClaimed;
        return dataItems() + index;
    }

    bool release(T const & item, size_t * foundIndex = nullptr) {
        return release(&item, foundIndex);
    }

    bool release(T const * item, size_t * foundIndex = nullptr) {
        if (!item) {
            return false;
        }
        assert ((byte_t *)item >= (byte_t *)dataItems() && "Invalid pointer.");
        size_t byteDiff = (byte_t *)item - (byte_t *)dataItems();
        size_t index = byteDiff / sizeof(T);
        assert (byteDiff % sizeof(T) == 0 && "Invalid pointer.");
        if (foundIndex) {
            *foundIndex = index;
        }
        return release(index);
    }

    bool release(size_t index) {
        assert(!isFree(index) && "Out of range.");
        // already free, don't relase
        if (freeList()->isFree(index)) {
            return false;
        }
        freeList()->release(index);
        dataItems()[index] = {};
        --_nClaimed;
        return true;
    }

    bool isFree(size_t index) const {
        assert(index < _size && "Out of range.");
        return freeList()->isFree(index);
    }

    bool isFull() const {
        return freeList()->isFull();
    }

    void reset() {
        freeList()->reset();
        for (size_t i = 0; i < _size; ++i) {
            dataItems()[i] = {};
        }
    }

    T const * dataItems() const { return (T const *)(dataBase() + sizeof(FreeList) + FreeList::DataSize(_size)); }

    #if DEBUG
    void print() {
        printl("Pool:");
        printl("    Pool size:  %zu", sizeof(Pool));
        printl("    DataSize:   %zu  (does not include Pool itself)", DataSize(_size));
        printl("    this:       %p", this);
        printl("    dataBase:   %p", dataBase());
        printl("    freeList:   %p", freeList());
        printl("    dataItem:   %p", dataItems());
    }
    #endif // DEBUG

private:
    size_t _size;
    size_t _nClaimed;

    byte_t       * dataBase()       { return (byte_t *)this + sizeof(Pool); }
    byte_t const * dataBase() const { return (byte_t *)this + sizeof(Pool); }

    FreeList       * freeList()       { return (FreeList *)dataBase(); }
    FreeList const * freeList() const { return (FreeList *)dataBase(); }

    T * dataItems() { return (T *)(dataBase() + sizeof(FreeList) + FreeList::DataSize(_size)); }

    #if DEV_INTERFACE
    friend void Pool_editorCreate();
    friend void Pool_editorEditBlock(Pool<int> &);
    #endif // DEV_INTERFACE
};

#if DEV_INTERFACE
void Pool_editorCreate();
void Pool_editorEditBlock(Pool<int> &);
#endif // DEV_INTERFACE
