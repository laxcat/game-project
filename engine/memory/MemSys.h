#pragma once
#include "../common/types.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
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
    #include "MemSys_Block.inc.h"

    // reference objects to internal memory --------------------------------- //

    // assumed to be uninitialized. has reserved space, but also has head (count)
    #include "MemSys_Array.inc.h"

    // assumed to be initalized, fixed size. no head.
    #include "MemSys_View.inc.h"

    // internal block content types ----------------------------------------- //

    #include "MemSys_Pool.inc.h"
    #include "MemSys_Stack.inc.h"
    // read only for now
    #include "MemSys_File.inc.h"


    // lifecycle ------------------------------------------------------------ //

    void init(size_t size);
    void shutdown();

    // create/destroy for internal types ------------------------------------ //

    Pool * createPool(size_t objCount, size_t objSize);
    void destroyPool(Pool * a);
    Stack * createStack(size_t size);
    void destroyStack(Stack * s);
    File * createFileHandle(char const * path, bool loadNow = false);
    void destroyFileHandle(File * f);
    template <typename T, typename ... TP>
        T * create(size_t size, TP && ... params);
    void destroy(void * ptr);

    // block handling ------------------------------------------------------- //

    // find block with enough free space, split it to size, and return it
    Block * requestFreeBlock(size_t size);

private:
    byte_t * _data = nullptr;
    Block * _blockHead = nullptr;
    size_t _size = 0;

    // check if random pointer is within
    bool isWithinData(byte_t * ptr, size_t size = 0);

    // debug ---------------------------------------------------------------- //
public:
    void getInfo(char * buf, int bufSize);
};

template <typename T, typename ... TP>
inline T * MemSys::create(size_t size, TP && ... params) {
    Block * block = requestFreeBlock(size + sizeof(T));
    if (!block) return nullptr;
    block->type = TYPE_EXTERNAL;
    return new (block->data()) T{static_cast<TP &&>(params)...};
}
