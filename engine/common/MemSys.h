#pragma once
#include "types.h"

class MemSys {
public:
    // types
    class Allocator {
    public:
        friend class MemSys;

        size_t size() const { return _objMaxCount * _objSize; }

        template <typename T>
        T * claimTo(size_t index) {
            assert(sizeof(T) == _objSize);
            if (index >= _objMaxCount) return nullptr;
            _freeIndex = index;
            return &((T *)_ptr)[_freeIndex];
        }

        template <typename T>
        T * claim(size_t count) {
            assert(sizeof(T) == _objSize);
            if (_freeIndex + count >= _objMaxCount) return nullptr;
            _freeIndex += count;
            return &((T *)_ptr)[_freeIndex];
        }

        void reset() { _freeIndex = 0; }

    private:
        byte_t * _ptr = nullptr;
        size_t _objMaxCount = 0;
        size_t _objSize = 0;
        Allocator * _next = nullptr;
        Allocator * _prev = nullptr;
        size_t _freeIndex = 0;

        Allocator(byte_t * ptr, size_t objCount, size_t objSize) {
            _ptr = ptr;
            _objMaxCount = objCount;
            _objSize = objSize;
        }
    };

    // public
    // Allocator stringFrameAllocator;

    // lifecycle
    void init(size_t size) {
        _data = (byte_t *)malloc(size);
        _size = size;
        _free = 0;
        auto fi = getFreeInfoFromData();
        fi->size = size;
        fi->prev = InvalidLoc;
        fi->next = InvalidLoc;
    }

    Allocator * createAllocator(size_t objCount, size_t objSize) {
        printf("createAllocator\n");

        byte_t * ptr = request(objCount * objSize + sizeof(Allocator));
        if (!ptr) return nullptr;

        printf("ptr for new allocator %p (_data = %p, loc = %zu)\n", ptr, _data, ptr - _data);

        auto a = new (ptr) Allocator{ptr + sizeof(Allocator), objCount, objSize};

        Allocator * prev = nullptr;
        for (auto p = _allocatorListHead; p; p = p->_next) {
            auto loc = (byte_t *)p;
            if (loc < ptr && loc + p->size() >= ptr) {
                prev = p;
                break;
            }
        }
        if (prev) {
            a->_prev = prev;
            a->_next = prev->_next;
            prev->_next = a;
        }

        return a;
    }

    void destroyAllocator(Allocator * a) {
        // check
        size_t aloc = (byte_t *)a - _data;
        if (aloc > _size) {
            fprintf(
                stderr,
                "Unexpected location for allocator. Expected mem range %p-%p, recieved %p",
                _data, _data + _size, a
            );
            assert(false);
        }

        // amount of space to freeing up
        size_t asize = sizeof(Allocator) + a->size();

        // link up prev/next allocators
        if (a->_prev) a->_prev->_next = a->_next;
        if (a->_next) a->_next->_prev = a->_prev;

        // run dtor
        a->~Allocator();

        // find prev free space
        FreeInfo * prevfi = nullptr;
        size_t floc = _free;
        while (floc != InvalidLoc) {
            FreeInfo * fi = getFreeInfoFromData(floc);
            if (!fi) break;
            if (floc < aloc && (fi->next == InvalidLoc || fi->next >= aloc)) {
                prevfi = fi;
                break;
            }
            floc = fi->next;
        }
        // find next free space
        FreeInfo * nextfi = getFreeInfoFromData((prevfi) ? prevfi->next : _free);
        // locations relative to _data
        size_t prevloc = (prevfi) ? (byte_t *)prevfi - _data : InvalidLoc;
        size_t nextloc = (nextfi) ? (byte_t *)nextfi - _data : InvalidLoc;

        // merge two free spaces on either side
        if (
            prevfi && // free space exists before
            nextfi && // free space exists after
            prevloc + prevfi->size == aloc && // prev space abuts target allocator
            aloc + asize == nextloc // target allocator abuts next space
        ) {
            prevfi->size += asize + nextfi->size;
            prevfi->next = nextfi->next;
            if (prevfi->next != InvalidLoc) getFreeInfoFromData(prevfi->next)->prev = prevloc;
        }
        // merge with prev
        else if (
            prevfi && // free space exists before
            prevloc + prevfi->size == aloc // prev space abuts target allocator
        ) {
            prevfi->size += asize;
        }
        // merge with next
        else if (
            nextfi && // free space exists after
            aloc + sizeof(Allocator) + a->size() == nextloc // target allocator abuts next space
        ) {
            FreeInfo * fi = getFreeInfoFromData(aloc);
            fi->size = asize + nextfi->size;
            fi->prev = (prevfi) ? prevloc : InvalidLoc;
            fi->next = nextfi->next;
            if (nextfi->next) getFreeInfoFromData(nextfi->next)->prev = aloc;
            if (prevfi) getFreeInfoFromData(prevloc)->next = aloc;
        }
        // create new free space
        else {
            FreeInfo * fi = getFreeInfoFromData(aloc);
            fi->size = asize;
            fi->prev = InvalidLoc;
            fi->next = InvalidLoc;
            if (prevfi) {
                fi->prev = prevloc;
                prevfi->next = aloc;
            }
            if (nextfi) {
                fi->next = nextloc;
                nextfi->prev = aloc;
            }
        }
    }

    void shutdown() {
        if (_data) free(_data), _data = nullptr;
        _free = 0;
    }
private:
    byte_t * _data = nullptr;
    Allocator * _allocatorListHead = nullptr;
    size_t _size = 0;
    size_t _free = InvalidLoc; // location of first FreeInfo written into the data

    struct FreeInfo {
        size_t size;
        size_t prev;
        size_t next;
    };

    static constexpr size_t MinFreeSpace = sizeof(FreeInfo);
    static constexpr size_t InvalidLoc = SIZE_MAX;

    // if requested space found:
    //     • updates _free and free info
    //     • returns pointer to requested free space
    // if requested space not found:
    //     • does not modify _free / free info
    //     • returns nullptr
    byte_t * request(size_t size) {
        FreeInfo * fi = getFreeInfoFromData();
        while(fi && fi->size < size) {
            fi = getFreeInfoFromData(fi->next);
        }
        if (!fi) {
            return nullptr;
        }

        auto claimed = (byte_t *)fi;

        // shrink this free space
        if (fi->size - size > MinFreeSpace) {
            // write new info in new location
            size_t newFree = (claimed - _data) + fi->size;
            auto nfi = getFreeInfoFromData(newFree);
            nfi->size = fi->size - size;
            nfi->prev = fi->prev;
            nfi->next = fi->next;
        }
        // consume this free space
        else {
            FreeInfo * prev = getFreeInfoFromData(fi->prev);
            FreeInfo * next = getFreeInfoFromData(fi->next);
            prev->next = fi->next;
            next->prev = fi->prev;
        }

        return claimed;
    }

    FreeInfo * getFreeInfoFromData(size_t loc = InvalidLoc) {
        // default to _free
        if (loc == InvalidLoc) loc = _free;
        // no free space
        if (loc == InvalidLoc) return nullptr;
        // return info written into data
        return (FreeInfo *)(_data + loc);
    }
};
