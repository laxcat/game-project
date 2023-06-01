#include "MemMan.h"
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <new>
#include "../common/file_utils.h"
#include "../dev/print.h"
#include "Stack.h"
#include "Pool.h"
#include "File.h"
#include "mem_align.h"
#include "../common/debug_defines.h"

#if 0 // enable/disable DEBUG_MSG
template <typename ...TS> void debugInfo(void *, char const *, TS && ...);
#define DEBUG_MSG(...) debugInfo(this, __VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

// some granular control of debug messages
constexpr static bool ShowMemManInfoDbg = false;
constexpr static bool ShowMemManBGFXDbg = false;


// lifecycle ------------------------------------------------------------ //

void MemMan::init(size_t size) {
    _data = (byte_t *)malloc(size);
    _size = size;
    #if DEBUG
    memset(_data, 0, _size);
    #endif // DEBUG

    printl("MEM MAN RANGE %*p—%*p", 8, _data, 8, _data+_size);

    _blockHead = new (_data) Block();
    _blockHead->_dataSize = size - BlockInfoSize;
    _blockTail = _blockHead;
    // _firstFree = _blockHead;
}

void MemMan::shutdown() {
    if (_data) free(_data), _data = nullptr;
}

// create/destroy for internal types ------------------------------------ //

Pool * MemMan::createPool(size_t objCount, size_t objSize) {
    Block * block = requestFreeBlock(objCount * objSize + sizeof(Pool));
    if (!block) return nullptr;
    block->_type = TYPE_POOL;
    // updateFirstFree(block);
    return new (block->data()) Pool{objCount, objSize};
}

void MemMan::destroyPool(Pool * a) {
    destroy(a);
}

Stack * MemMan::createStack(size_t size) {
    Block * block = requestFreeBlock(size + sizeof(Stack));
    if (!block) return nullptr;
    block->_type = TYPE_STACK;
    // updateFirstFree(block);
    return new (block->data()) Stack{size};
}

void MemMan::destroyStack(Stack * s) {
    destroy(s);
}

File * MemMan::createFileHandle(char const * path, bool loadNow) {
    // open to deternmine size, and also maybe load
    errno = 0;
    FILE * fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error loading file \"%s\": %d\n", path, errno);
        return nullptr;
    }
    long fileSize = getFileSize(fp);
    if (fileSize == -1) {
        fprintf(stderr, "Error reading seek position in file \"%s\": %d\n", path, errno);
        return nullptr;
    }

    // make size one bigger. load process will write 0x00 in the last byte
    // after file contents so contents can be printed as string in place.
    size_t size = (size_t)fileSize + 1;

    Block * block = requestFreeBlock(size + sizeof(File));
    if (!block) return nullptr;
    block->_type = TYPE_FILE;
    // updateFirstFree(block);
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

void MemMan::destroyFileHandle(File * f) {
    destroy(f);
}

// MemMan::Entity * MemMan::createEntity(char const * path, bool loadNow) {
//     File * f = createFileHandle(path, true);


//     // // open to deternmine size, and also maybe load
//     // errno = 0;
//     // FILE * fp = fopen(path, "r");
//     // if (!fp) {
//     //     fprintf(stderr, "Error loading file \"%s\": %d\n", path, errno);
//     //     return nullptr;
//     // }

//     // // calc size
//     // auto counter = Entity::getMemorySize(fp);
//     // size_t gltfSize = counter.totalSize();
//     // printl("gltf %s memory size %zu", path, gltfSize);

//     // // create block from size
//     // Block * block = requestFreeBlock(gltfSize + sizeof(Entity));
//     // if (!block) return nullptr;
//     // block->type = TYPE_ENTITY;
//     // Entity * entity = new (block->data()) Entity{path};

//     // // seek back to start of file
//     // int fseekError = fseek(fp, 0L, 0);
//     // if (fseekError) {
//     //     fprintf(stderr, "Error seeking in file \"%s\": %d\n", path, fseekError);
//     //     return nullptr;
//     // }

//     // // load it into memory
//     // entity->load(fp, entity->data(), gltfSize, counter);

//     // // close fp
//     // fclose(fp);

//     // entity->printGLTF();

//     return nullptr;
// }

// void MemMan::destroyEntity(Entity * f) {
//     destroy(f);
// }

void MemMan::destroy(void * ptr) {
    guard_t guard{mainMutex};

    assertWithinData((byte_t *)ptr);

    // we verified the ptr is in our expected memory space,
    // so we expect block info just prior to it
    Block * block = blockForDataPtr(ptr);
    #if DEBUG
    if (!isValidBlock(block)) printInfo();
    assert(isValidBlock(block) && "Block not valid. (destroy)");
    #endif // DEBUG
    // set block to free
    block->_type = TYPE_FREE;
    // updateFirstFree(block);
    // zero out data (DEBUG)
    #if DEBUG
    memset(block->data(), 0, block->dataSize());
    assert(isValidBlock(block) && "Block not valid. (destroy)");
    #endif // DEBUG
    // try to merge with neighboring blocks
    mergeFreeBlocks(block);
    if (block->_prev) {
        #if DEBUG
        assert(isValidBlock(block->_prev) && "Block not valid. (destroy)");
        #endif // DEBUG
        mergeFreeBlocks(block->_prev);
    }
}

// block handling ------------------------------------------------------- //

MemMan::Block * MemMan::findFree(size_t size) {
    guard_t guard{mainMutex};

    Block * block = nullptr;
    int i = 0;
    for (block = _blockHead; block; block = block->_next) {
        if (block->_type == TYPE_FREE && block->dataSize() >= size) {
            break;
        }
        ++i;
    }
    DEBUG_MSG("findFree looked at %d blocks", i);
    if (!block) {
        DEBUG_MSG("NO FREE BLOCK for size: %zu", size);
        return nullptr;
    }

    #if DEBUG
    assert(isValidBlock(block) && "Block not valid. (findFree)");
    #endif // DEBUG

    return block;
}

// find block with enough free space, split it to size, and return it
MemMan::Block * MemMan::requestFreeBlock(size_t size) {
    guard_t guard{mainMutex};

    DEBUG_MSG("REQUEST FREE BLOCK start");

    // TODO: reintroduce better alignment policy with broader scope
    // size = ALIGN(size);

    Block * block = findFree(size);
    if (block == nullptr) return nullptr;

    // it might be a few cycles until actual type is set, so to be absolutely
    // safe and prevent multiple threads claiming a free block...
    block->_type = TYPE_CLAIMED;

    // We don't care if the split actually happens or not.
    // If it happened, we still return the first block and the 2nd is of no
    // concern.
    // If it didn't, it was the rare case where the block is big enough to
    // hold our data, but with not enough room to create a new block.
    splitBlock(block, size);

    DEBUG_MSG("REQUEST FREE BLOCK end");

    // the old block is returned.
    // it was (probably) shrunk to fit by splitBlock.
    return block;
}

MemMan::Block * MemMan::splitBlock(Block * blockA, size_t blockANewSize) {
    guard_t guard{mainMutex};

    DEBUG_MSG("SPLIT BLOCK start");

    #if DEBUG
    assert(isValidBlock(blockA) && "BlockA not valid. (splitBlock)");
    #endif // DEBUG

    // blockA is not big enough.
    // equal data size also rejected because blockB would have
    // 0 bytes for data, and that would be pointless.
    if (blockA->dataSize() <= blockANewSize + BlockInfoSize) {
        DEBUG_MSG("SPLIT BLOCK block A not big enough");
        return nullptr;
    }

    // where to put the new block?
    byte_t * newLoc = blockA->data() + blockANewSize;
    // write block info into data space with defaults
    Block * blockB = new (newLoc) Block();
    // block blockB gets remaining space
    blockB->_dataSize = blockA->dataSize() - BlockInfoSize - blockANewSize;

    // set this size
    blockA->_dataSize = blockANewSize;

    // link up
    blockB->_next = blockA->_next;
    blockB->_prev = blockA;
    blockA->_next = blockB;
    if (_blockTail == blockA) _blockTail = blockB;

    #if DEBUG
    memset(blockB->data(), 0, blockB->dataSize());
    assert(isValidBlock(blockA) && "BlockA not valid. (splitBlock)");
    assert(isValidBlock(blockB) && "BlockB not valid. (splitBlock)");
    #endif // DEBUG

    DEBUG_MSG("SPLIT BLOCK end");

    return blockA;
}

MemMan::Block * MemMan::splitBlockEnd(Block * blockA, size_t blockBNewSize) {
    guard_t guard{mainMutex};

    if (blockBNewSize + BlockInfoSize > blockA->dataSize()) return nullptr;
    auto ret = splitBlock(blockA, blockA->dataSize() - BlockInfoSize - blockBNewSize);
    return (ret) ? ret->_next : nullptr;
}

MemMan::Block * MemMan::mergeFreeBlocks(Block * block) {
    guard_t guard{mainMutex};

    DEBUG_MSG("MERGE BLOCK start");

    #if DEBUG
    // printInfo();
    assert(isValidBlock(block) && "Block not valid. (mergeFreeBlocks)");
    #endif // DEBUG

    // fail and bail if any these conditions are true
    if (block->_type != TYPE_FREE ||
        block->_next == nullptr ||
        block->_next->_type != TYPE_FREE
    ) {
        DEBUG_MSG("MERGE BLOCK failed");
        return nullptr;
    }

    #if DEBUG
    assert(isValidBlock(block->_next) && "Block->_next not valid. (mergeFreeBlocks)");
    #endif // DEBUG

    // expand block to encompas next block, which is free
    block->_dataSize += block->_next->totalSize();

    // relink everything
    if (block->_next == _blockTail) _blockTail = block;
    block->_next = block->_next->_next;
    if (block->_next) block->_next->_prev = block;

    #if DEBUG
    memset(block->data(), 0, block->dataSize());
    assert(isValidBlock(block) && "Block not valid. (mergeFreeBlocks after linkage)");
    if (block->_prev) {
        assert(isValidBlock(block->_prev) &&
            "Block->_prev not valid. (mergeFreeBlocks after linkage)");
    }
    if (block->_next) {
        assert(isValidBlock(block->_next) &&
            "Block->_next not valid. (mergeFreeBlocks after linkage)");
    }
    #endif // DEBUG

    // updateFirstFree(block);

    DEBUG_MSG("MERGE BLOCK end");

    return block;
}

MemMan::Block * MemMan::resizeBlock(Block * block, size_t newSize) {
    guard_t guard{mainMutex};

    DEBUG_MSG("RESIZE BLOCK start");

    #if DEBUG
    assert(isValidBlock(block) && "Block not valid. (resizeBlock)");
    #endif // DEBUG

    // working
    Block * newBlock = nullptr;
    bool nextBlockFree = (block->_next && block->_next->_type == TYPE_FREE);

    // do nothing; same size already
    if (newSize == block->dataSize()) {
        DEBUG_MSG("RESIZE BLOCK do nothing; same size: %zu", newSize);
        return block;
    }
    // do nothing; shrinking; not enough room to create new block, and next block not free
    else if (
        !nextBlockFree &&
        newSize < block->dataSize() &&
        block->dataSize() - newSize <= BlockInfoSize
    ) {
        DEBUG_MSG("RESIZE BLOCK do nothing; shrinking by too little. "
            "oldSize: %zu, newSize: %zu", block->dataSize(), newSize);
        return block;
    }
    // shrink block; create new free block with extra space
    else if (block->dataSize() >= newSize + BlockInfoSize) {
        Block * shrunkBlock = splitBlock(block, newSize);
        // splitBlock may have created a new free block; make sure it's merged if possible.
        // mergeFreeBlocks will check if blocks are free
        if (shrunkBlock->_next) {
            mergeFreeBlocks(shrunkBlock->_next);
        }
        DEBUG_MSG("RESIZE BLOCK shrink block");
        return shrunkBlock;
    }
    // expand block in place by eating into adjacent free block
    else if (
        block->_next &&
        block->_next->_type == TYPE_FREE &&
        // newSize > block->dataSize() && // redundant check
        newSize <= block->dataSize() + block->_next->totalSize()
    ) {
        // how much are we taking from adjacent free block
        size_t deltaSize = newSize - block->dataSize();

        // might fail, but we will consume all of block->_next either way.
        // either a split happened and we consume the exact requested amount,
        // or a split didn't happen and we consume the entire block with a little extra space in it.
        splitBlock(block->_next, deltaSize);

        // expand block
        block->_dataSize += block->_next->totalSize();

        // hang onto old next location
        #if DEBUG
        byte_t * oldNext = (byte_t *)block->_next;
        #endif // DEBUG

        // link up
        block->_next = block->_next->_next;
        if (block->_next) block->_next->_prev = block;

        // zero out old block info
        #if DEBUG
        memset(oldNext, 0, BlockInfoSize);
        assert(isValidBlock(block) && "Block not valid. (resize expand in place)")
        #endif // DEBUG

        DEBUG_MSG("RESIZE BLOCK expand in place");

        // updateFirstFree(block);

        return block;
    }
    // move block and free old location
    else if ((newBlock = requestFreeBlock(newSize))) {
        // update info of new block
        newBlock->_type = block->_type;
        // updateFirstFree(newBlock);

        // copy to new place (frees old block, returns new block)
        memcpy(newBlock->data(), block->data(), block->dataSize());
        destroy(block->data());

        DEBUG_MSG("RESIZE BLOCK copy to new place");

        #if DEBUG
        assert(isValidBlock(newBlock) && "Block not valid. (resize expand in place)");
        #endif // DEBUG

        return newBlock;
    }

    DEBUG_MSG("RESIZE BLOCK failed\n%s");

    // failed
    return nullptr;
}

MemMan::Block * MemMan::blockForDataPtr(void * ptr) {
    guard_t guard{mainMutex};

    Block * block = (Block *)((byte_t *)ptr - BlockInfoSize);

    #if DEBUG
    assert(isValidBlock(block) && "Block not valid. (blockForData)");
    #endif // DEBUG

    return block;
}

MemMan::Block const * MemMan::firstBlock() const {
    return _blockHead;
}

MemMan::Block const * MemMan::nextBlock(Block const & block) const {
    return block._next;
}


void MemMan::assertWithinData(void * ptr, size_t size) const {
    auto bptr = (byte_t *)ptr;
    // within range
    if (bptr >= _data && bptr + size <= _data + _size) {
        return;
    }
    // not in range
    fprintf(
        stderr,
        "Unexpected location for pointer. Expected mem range %p-%p, recieved %p\n",
        _data, _data + _size, bptr
    );
    assert(false);
}

// void MemMan::updateFirstFree (Block * block) {
//     guard_t guard{mainMutex};

//     if (block == _firstFree && block->type() != TYPE_FREE) {
//         for (Block * b = block->_next; b; b = b->_next) {
//             if (b->_type == TYPE_FREE) {
//                 _firstFree = b;
//                 break;
//             }
//         }
//     }
//     else if ((byte_t *)block < (byte_t *)_firstFree) {
//         _firstFree = block;
//     }
//     // #if DEBUG
//     // int i = 0;
//     // for (Block * b = _blockHead; b; b = b->_next, ++i) {
//     //     if (b == _firstFree) {
//     //         printl("New _firstFree: %d", i);
//     //         break;
//     //     }
//     // }
//     // #endif // DEBUG
// }

bool MemMan::isValidBlock(Block * block) const {
    guard_t guard{mainMutex};

    if (!block) return false;
    if (memcmp(&BlockMagicString, block->_info, 4) != 0) return false;
    if (!block->_next && block != _blockTail) return false;
    if (!block->_prev && block != _blockHead) return false;
    if (block->_dataSize == 0) return false;
    if ((byte_t *)block + block->totalSize() > _data + _size) return false;
    return true;
}


void * memManAlloc(size_t size, void * userData) {
    if (!size || !userData) return nullptr;
    MemMan * memMan = (MemMan *)userData;
    MemMan::Block * block = memMan->requestFreeBlock(size);
    if (!block) {
        fprintf(stderr, "Unable to alloc memory: %zu\n", size);
        assert(false);
    }
    block->_type = MemMan::TYPE_EXTERNAL;
    // memMan->updateFirstFree(block);
    return block->data();
}

void * memManRealloc(void * ptr, size_t size, void * userData) {
    if (ptr == nullptr) return memManAlloc(size, userData);
    if (size == 0) {
        memManFree(ptr, userData);
        return nullptr;
    }
    if (!userData) return nullptr;
    MemMan * memMan = (MemMan *)userData;
    memMan->assertWithinData(ptr, size);
    auto block = (MemMan::Block *)((byte_t *)ptr - MemMan::BlockInfoSize);

    #if DEBUG
    assert(memMan->isValidBlock(block) && "Block not valid. (memManRealloc)");
    #endif // DEBUG

    block = memMan->resizeBlock(block, size);
    return (block) ? block->data() : nullptr;
}

void memManFree(void * ptr, void * userData) {
    if (!ptr || !userData) return;
    MemMan * memMan = (MemMan *)userData;
    memMan->destroy(ptr);
}

void * BXAllocator::realloc(void * ptr, size_t size, size_t align, char const * file, uint32_t line) {
    if constexpr (ShowMemManBGFXDbg) print("BGFX ALLOC: "
        "%p, "
        "%*zu, "
        "%*zu, "
        "%s:%d ",
        ptr,
        10, size,
        2, align,
        file,
        line
    );

    #if DEBUG
        assert(memMan && "Memory manager pointer not set in BXAllocator.");
    #endif

    void * ret = memManRealloc(ptr, size, (void *)memMan);
    if (ptr == nullptr && size) {
        memMan->blockForDataPtr(ret)->_type = MemMan::TYPE_BGFX;
    }

    if constexpr (ShowMemManBGFXDbg) printl("RETURN: %*p", 8, ret);
    return ret;
}

// debug ---------------------------------------------------------------- //

template <typename ...TS>
void debugInfo(void * userData, char const * prefixMsg, TS && ... params) {
    auto memMan = (MemMan *)userData;
    if constexpr (ShowMemManInfoDbg) memMan->printInfo(prefixMsg, std::forward<TS>(params)...);
    else printl(prefixMsg, std::forward<TS>(params)...);
}

void MemMan::printInfo(char const * prefixMsg) {
    #if DEBUG
        // constexpr static size_t InfoStrSz = 1024*1024;
        constexpr static size_t InfoStrSz = 1024*1024;
    #else
        constexpr static size_t InfoStrSz = 0;
        if (prefixMsg == nullptr) return;
    #endif // DEBUG

    char infoStr[InfoStrSz];

    getInfo(infoStr, InfoStrSz);

    if (prefixMsg)  printl("%s\n%s", prefixMsg, infoStr);
    else            printl("%s", infoStr);
}

void MemMan::getInfo(char * buf, int bufSize) {
    if (bufSize <= 0) return;

    int blockCount = 0;
    for (Block * b = _blockHead; b; b = b->_next) {
        ++blockCount;
    }

    int wrote = 0;
    wrote += snprintf(
        buf + wrote,
        bufSize - wrote,
        "MemMan\n"
        "================================================================================\n"
        "Data: %p, Size: %zu, Block count: %d\n"
        ,
        _data,
        _size,
        blockCount
    );

    int i = 0;
    for (Block * b = _blockHead; b != nullptr; b = b->_next, ++i) {
        if (bufSize <= 0) return;

        wrote += snprintf(
            buf + wrote,
            bufSize - wrote,
            "--------------------------------------------------------------------------------\n"
            "Block %d, %s\n"
            "--------------------\n"
            "Base: %p, Data: %p, BlockDataSize: %zu\n"
            ,
            i,
                (b->_type == TYPE_FREE)     ? "FREE"  :
                (b->_type == TYPE_POOL)     ? "POOL"  :
                (b->_type == TYPE_STACK)    ? "STACK" :
                (b->_type == TYPE_FILE)     ? "FILE" :
                (b->_type == TYPE_EXTERNAL) ? "EXTERNAL" :
                "(unknown type)"
            ,
            b,
            b->data(),
            b->dataSize()
        );

        if (b->_type == TYPE_FREE) {
        }
        else if (b->_type == TYPE_POOL) {
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
        else if (b->_type == TYPE_STACK) {
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
        else if (b->_type == TYPE_FILE) {
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
        else if (b->_type == TYPE_EXTERNAL) {
        }

        wrote += snprintf(
            buf + wrote,
            bufSize - wrote,
            "--------------------------------------------------------------------------------\n"
        );
    }
}
