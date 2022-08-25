#pragma once
#include "types.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <new>

class MemSys {
public:
    // types ---------------------------------------------------------------- //

    enum Type {
        // empty space to be claimed
        TYPE_FREE,
        // internal types, with special treatment
        TYPE_POOL,
        TYPE_STACK,
        TYPE_FILE,
        // externally requested of any type
        TYPE_EXTERNAL
    };

    // Block, basic unit of all sub sections of the memory space.
    //
    // Each block will be placed in the large memory space, with an instance
    // placed at the start of the block, with "size" bytes available directly
    // after in memory. Member "size" represents this data space size, so the
    // block actually takes up size + BlockInfoSize in MemSys data block.
    class Block {
    public:
        friend class MemSys;

        size_t dataSize() const { return size; }
        size_t totalSize()  const { return BlockInfoSize + size; }
        byte_t * data() { return (byte_t *)this + BlockInfoSize; }

    private:
        size_t size = 0; // this is data size, not total size!
        Block * prev = nullptr;
        Block * next = nullptr;
        Type type = TYPE_FREE;

        // "info" could maybe be a magic string (for safety checks). or maybe additional info.
        // 8-byte allignment on current machine is forcing this Block to always be 32 bytes anyway
        // so this is here to make that explicit. We could even take more space from type, which
        // could easily be only 1 or 2 bytes.
        byte_t info[4];

        Block() {}

        // split block A into block A (with requested data size) and block B with what remains.
        Block * split(size_t blockANewSize) {
            // block a (this) is not big enough.
            // equal data size also rejected because new block would have
            // 0 bytes for data.
            if (dataSize() <= blockANewSize + BlockInfoSize) return nullptr;

            // where to put the new block?
            byte_t * newLoc = data() + blockANewSize;
            // write block info into data space with defaults
            Block * b = new (newLoc) Block();
            // block b gets remaining space
            b->size = dataSize() - BlockInfoSize - blockANewSize;

            // set this size
            size = blockANewSize;

            // link up
            b->next = next;
            b->prev = this;
            next = b;

            return b;
        }

        // merge this block with next block IF both are free
        Block * mergeWithNext() {
            if (type != TYPE_FREE || !next || next->type != TYPE_FREE) return nullptr;
            size += next->totalSize();
            next = next->next;
            return this;
        }
    };
    constexpr static size_t BlockInfoSize = sizeof(Block);

    // reference objects to internal memory --------------------------------- //

    // assumed to be uninitialized. has reserved space, but also has head (count)
    template <typename T>
    class Array {
    public:
        Array() : _ptr(nullptr), _max(0) {}
        Array(T * ptr, size_t max) : _ptr(ptr), _max(max) {}

        T * claimTo(size_t i) {
            assert(i > _count && "Index must be greated than count");
            T * r = &_ptr[_count];
            _count = i;
            return r;
        }

        T * claim(size_t i = 1) { return claimTo(_count + i); }

        // TODO: copy assignment or memcpy?
        void append(T const & obj) {
            *claim() = obj;
            // memcpy(claim(), &obj, sizeof(T));
        }

        size_t count() const { return _count; }
        size_t max() const { return _max; }
        bool isValid() const { return _ptr != nullptr; }

        T & operator[](size_t i) { assert(_ptr && i < _count); return _ptr[i]; }
        T const & operator[](size_t i) const { assert(_ptr && i < _count); return _ptr[i]; }

    private:
        T * _ptr;
        size_t _max;
        size_t _count = 0;
    };

    // assumed to be initalized, fixed size. no head.
    template <typename T>
    class View {
    public:
        View() : _ptr(nullptr), _count(0) {}
        View(T * ptr, size_t count) : _ptr(ptr), _count(count) {}
        size_t count() const { return _count; }
        bool isValid() const { return _ptr != nullptr && _count > 0; }
        T & operator[](size_t i) { assert(_ptr && i < _count); return _ptr[i]; }
        T const & operator[](size_t i) const { assert(_ptr && i < _count); return _ptr[i]; }

    private:
        T * _ptr;
        size_t _count;
    };

    // internal block content types ----------------------------------------- //

    class Pool {
    public:
        friend class MemSys;

        size_t size() const { return _objMaxCount * _objSize; }
        size_t objMaxCount() const { return _objMaxCount; }
        size_t objSize() const { return _objSize; }
        size_t freeIndex() const { return _freeIndex; }
        byte_t * data() const { return (byte_t *)this + sizeof(Pool); }

        // reserve up to (but not including) index from current head and return array.
        // doesn't write anything into the space. (TODO: zero init space?)
        template <typename T, typename ... TP>
        Array<T> reserveTo(size_t index, TP && ... params) {
            // checks
            if (sizeof(T) != _objSize) {
                fprintf(stderr, "Could not claim, size of T (%zu) does not match _objSize (%zu).",
                    sizeof(T), _objSize);
                assert(false);
            }
            if (index >= _objMaxCount) {
                fprintf(stderr, "Could not claim, index (%zu) out of range (max: %zu).",
                    index, _objMaxCount);
                return Array<T>{};
            }
            if (index <= _freeIndex) {
                fprintf(stderr, "Could not claim, freeIndex (%zu) already greater than requested index (%zu).",
                    _freeIndex, index);
                return Array<T>{};
            }

            // return start of array, at current head
            T * ptr = at<T>(_freeIndex);

            // calc count and move head
            size_t reserveCount = index - _freeIndex;
            _freeIndex = index;

            return Array{ptr, reserveCount};
        }

        // claim up to (but not including) index from current head and return view
        // do a placement new in each newly claimed slot
        template <typename T, typename ... TP>
        View<T> claimTo(size_t index, TP && ... params) {
            Array<T> a = reserveTo<T>(index);
            if (!a.isValid()) return View<T>{};
            while (a.count() < a.max()) {
                new (a.claim()) T{static_cast<TP&&>(params)...};
            }
            return View<T>{&a[0], a.max()};
        }

        // claim count number of objects and return start of array
        template <typename T, typename ... TP>
        View<T> claim(size_t count = 1, TP && ... params) {
            return claimTo<T>(_freeIndex + count, static_cast<TP&&>(params)...);
        }

        template <typename T>
        T * at(size_t index) {
            assert(index < _objMaxCount && "Index out of range.");
            return (T *)(data() + _objSize * index);
        }
        template <typename T> T * at() { return at<T>(_freeIndex); }

        void reset() { _freeIndex = 0; }

    private:
        size_t _objMaxCount = 0;
        size_t _objSize = 0;
        size_t _freeIndex = 0;

        Pool(size_t objCount, size_t objSize) :
            _objMaxCount(objCount),
            _objSize(objSize)
        {
        }
    };

    class Stack {
    public:
        friend class MemSys;

        size_t size() const { return _size; }
        size_t head() const { return _head; }
        byte_t * data() const { return (byte_t *)this + sizeof(Stack); }
        byte_t * dataHead() const { return data() + _head; }

        template <typename T>
        T * alloc(size_t count = 1) {
            if (_head + sizeof(T) * count > _size) return nullptr;
            T * ret = (T *)dataHead();
            _head += sizeof(T) * count;
            return ret;
        }

        byte_t * alloc(size_t size) {
            return alloc<byte_t>(size);
        }

        char * formatStr(char const * fmt, ...) {
            char * str = (char *)dataHead();
            va_list args;
            va_start(args, fmt);
            int written = vsnprintf(str, _size - _head, fmt, args);
            va_end(args);

            // vsnprintf writes '\0' to buffer but doesn't count it in return value
            if (written) _head += written + 1;

            if (_head > _size) _head = _size;
            return str;
        }

        void reset() { _head = 0; }

    private:
        size_t _size = 0;
        size_t _head = 0;

        Stack(size_t size) :
            _size(size)
        {
        }
    };

    // read only for now
    class File {
    public:
        friend class MemSys;

        // data size is one bigger than actual file size. 0x00 byte written at end.
        size_t size() const { return _size; }
        size_t head() const { return _head; }
        size_t fileSize() const { return _size - 1; }
        byte_t * data() const { return (byte_t *)this + sizeof(File); }
        byte_t * dataHead() const { return data() + _head; }
        bool loaded() const { return _loaded; }
        char const * path() const { return _path; }

        bool load(FILE * externalFP = nullptr) {
            FILE * fp;

            // if file was already open, user might have passed in a file pointer
            // to save from reopening it
            if (externalFP) {
                fp = externalFP;
                int fseekError = fseek(fp, 0L, 0);
                if (fseekError) {
                    fprintf(stderr, "Error seeking to begining of file \"%s\" for load: %d\n", _path, fseekError);
                    return false;
                }
            }
            else {
                errno = 0;
                fp = fopen(_path, "r");
                if (!fp) {
                    fprintf(stderr, "Error opening file \"%s\" for loading: %d\n", _path, errno);
                    return false;
                }
            }

            size_t readSize = fread(data(), 1, fileSize(), fp);
            if (readSize != fileSize()) {
                fprintf(stderr, "Error reading file \"%s\" contents: read %zu, expecting %zu\n",
                    _path, readSize, fileSize());
                return false;
            }

            if (!externalFP) {
                fclose(fp);
                int fe = ferror(fp);
                if (fe) {
                    fprintf(stderr, "Error closing file \"%s\" after load: %d\n", _path, fe);
                    return false;
                }
            }

            //0x00 byte written after file contents
            data()[_size-1] = '\0';

            _loaded = true;
            return true;
        }

    private:
        size_t _size = 0;
        size_t _head = 0;
        char const * _path = "";
        bool _loaded = false;

        File(size_t size, char const * path) :
            _size(size),
            _path(path)
        {
        }
    };

    // lifecycle ------------------------------------------------------------ //

    void init(size_t size) {
        _data = (byte_t *)malloc(size);
        _size = size;
        _blockHead = new (_data) Block();
        _blockHead->size = size - BlockInfoSize;
    }

    void shutdown() {
        if (_data) free(_data), _data = nullptr;
    }

    // create/destroy for internal types ------------------------------------ //

    Pool * createPool(size_t objCount, size_t objSize) {
        Block * block = requestFreeBlock(objCount * objSize + sizeof(Pool));
        if (!block) return nullptr;
        block->type = TYPE_POOL;
        return new (block->data()) Pool{objCount, objSize};
    }

    void destroyPool(Pool * a) {
        destroy(a);
    }

    Stack * createStack(size_t size) {
        Block * block = requestFreeBlock(size + sizeof(Stack));
        if (!block) return nullptr;
        block->type = TYPE_STACK;
        return new (block->data()) Stack{size};
    }

    void destroyStack(Stack * s) {
        destroy(s);
    }

    File * createFileHandle(char const * path, bool loadNow = false) {
        // open to deternmine size, and also maybe load
        errno = 0;
        FILE * fp = fopen(path, "r");
        if (!fp) {
            fprintf(stderr, "Error loading file \"%s\": %d\n", path, errno);
            return nullptr;
        }
        int fseekError = fseek(fp, 0L, SEEK_END);
        if (fseekError) {
            fprintf(stderr, "Error seeking in file \"%s\": %d\n", path, fseekError);
            return nullptr;
        }
        errno = 0;
        long fileSize = ftell(fp);
        if (fileSize == -1) {
            fprintf(stderr, "Error reading seek position in file \"%s\": %d\n", path, errno);
            return nullptr;
        }

        // make size one bigger. load process will write 0x00 in the last byte
        // after file contents so contents can be printed as string in place.
        size_t size = (size_t)fileSize + 1;

        Block * block = requestFreeBlock(size + sizeof(File));
        if (!block) return nullptr;
        block->type = TYPE_FILE;
        File * f = new (block->data()) File{size, path};

        // load now if request. send fp to avoid opening twice
        if (loadNow) {
            bool success = f->load(fp);
            // not sure what to do here. block successfully created but error in load...
            // keep block creation or cancel the whole thing?
            // File::load should report actual error.
            if (!success) {
            }
        }

        // close fp
        fclose(fp);

        return f;
    }

    void destroyFileHandle(File * f) {
        destroy(f);
    }

    template <typename T, typename ... TP>
    T * create(size_t size, TP && ... params) {
        Block * block = requestFreeBlock(size + sizeof(T));
        if (!block) return nullptr;
        block->type = TYPE_EXTERNAL;
        return new (block->data()) T{static_cast<TP &&>(params)...};
    }

    void destroy(void * ptr) {
        if (!isWithinData((byte_t *)ptr)) {
            fprintf(
                stderr,
                "Unexpected location for pointer. Expected mem range %p-%p, recieved %p",
                _data, _data + _size, ptr
            );
            assert(false);
        }

        // we verified the ptr is in our expected memory space,
        // so we expect block info just prior to it
        Block * block = (Block *)((byte_t *)ptr - BlockInfoSize);
        // set block to free
        block->type = TYPE_FREE;
        // try to merge with neighboring blocks
        block->mergeWithNext();
        if (block->prev) block->prev->mergeWithNext();
    }

    // block handling ------------------------------------------------------- //

    // find block with enough free space, split it to size, and return it
    Block * requestFreeBlock(size_t size) {
        Block * block = nullptr;
        for (block = _blockHead; block; block = block->next) {
            if (block->type == TYPE_FREE && block->dataSize() >= size) {
                break;
            }
        }
        if (!block) return nullptr;

        // We don't care if the split actually happens or not.
        // If it happened, we still return the first block and the 2nd is of no
        // concern.
        // If it didn't, it was the rare case where the block is big enough to
        // hold our data, but with not enough room to create a new block.
        block->split(size);

        // the old block gets returned. it was (probably) shrunk to fit by split
        return block;
    }

private:
    byte_t * _data = nullptr;
    Block * _blockHead = nullptr;
    size_t _size = 0;

    // check if random pointer is within
    bool isWithinData(byte_t * ptr, size_t size = 0) {
        return (ptr >= _data && ptr + size <= _data + _size);
    }

public:
    void getInfo(char * buf, int bufSize) {
        int blockCount = 0;
        for (Block * b = _blockHead; b; b = b->next) {
            ++blockCount;
        }

        int wrote = 0;
        wrote += snprintf(
            buf + wrote,
            bufSize - wrote,
            "MemSys\n"
            "================================================================================\n"
            "Data: %p, Size: %zu, Block count: %d\n"
            ,
            _data,
            _size,
            blockCount
        );

        int i = 0;
        for (Block * b = _blockHead; b != nullptr; b = b->next, ++i) {
            wrote += snprintf(
                buf + wrote,
                bufSize - wrote,
                "--------------------------------------------------------------------------------\n"
                "Block %d, %s\n"
                "--------------------\n"
                "Base: %p, Data: %p, BlockDataSize: %zu\n"
                ,
                i,
                    (b->type == TYPE_FREE)     ? "FREE"  :
                    (b->type == TYPE_POOL)     ? "POOL"  :
                    (b->type == TYPE_STACK)    ? "STACK" :
                    (b->type == TYPE_FILE)     ? "FILE" :
                    (b->type == TYPE_EXTERNAL) ? "EXTERNAL" :
                    "(unknown type)"
                ,
                b,
                b->data(),
                b->dataSize()
            );

            if (b->type == TYPE_FREE) {
            }
            else if (b->type == TYPE_POOL) {
                Pool * pool = (Pool *)b->data();
                wrote += snprintf(
                    buf + wrote,
                    bufSize - wrote,
                    "PoolInfoSize: %zu, ObjSize: %zu, MaxCount/DataSize: *%zu=%zu, FirstFree: %zu\n"
                    ,
                    sizeof(Pool),
                    pool->objSize(),
                    pool->objMaxCount(),
                    pool->size(),
                    pool->freeIndex()
                );
            }
            else if (b->type == TYPE_STACK) {
                Stack * stack = (Stack *)b->data();
                wrote += snprintf(
                    buf + wrote,
                    bufSize - wrote,
                    "StackInfoSize: %zu, DataSize: %zu, Head: %zu\n"
                    ,
                    sizeof(Stack),
                    stack->size(),
                    stack->head()
                );
            }
            else if (b->type == TYPE_FILE) {
                File * file = (File *)b->data();
                wrote += snprintf(
                    buf + wrote,
                    bufSize - wrote,
                    "FileInfoSize: %zu, FileSize: %zu, DataSize: %zu, Head: %zu, Loaded: %s\n"
                    "Path: %s\n"
                    ,
                    sizeof(File),
                    file->fileSize(),
                    file->size(),
                    file->head(),
                    file->loaded()?"yes":"no",
                    file->path()
                );
            }
            else if (b->type == TYPE_EXTERNAL) {
            }

            wrote += snprintf(
                buf + wrote,
                bufSize - wrote,
                "--------------------------------------------------------------------------------\n"
            );
        }
    }
};
