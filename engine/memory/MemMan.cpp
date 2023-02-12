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

// lifecycle ------------------------------------------------------------ //

void MemMan::init(size_t size) {
    _data = (byte_t *)malloc(size);
    _size = size;
    _blockHead = new (_data) Block();
    _blockHead->_size = size - BlockInfoSize;
    _blockTail = _blockHead;
}

void MemMan::shutdown() {
    if (_data) free(_data), _data = nullptr;
}

// create/destroy for internal types ------------------------------------ //

Pool * MemMan::createPool(size_t objCount, size_t objSize) {
    Block * block = requestFreeBlock(objCount * objSize + sizeof(Pool));
    if (!block) return nullptr;
    block->_type = TYPE_POOL;
    return new (block->data()) Pool{objCount, objSize};
}

void MemMan::destroyPool(Pool * a) {
    destroy(a);
}

Stack * MemMan::createStack(size_t size) {
    Block * block = requestFreeBlock(size + sizeof(Stack));
    if (!block) return nullptr;
    block->_type = TYPE_STACK;
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
    assertWithinData((byte_t *)ptr);

    // we verified the ptr is in our expected memory space,
    // so we expect block info just prior to it
    Block * block = (Block *)((byte_t *)ptr - BlockInfoSize);
    // set block to free
    block->_type = TYPE_FREE;
    // try to merge with neighboring blocks
    mergeBlockWithNext(block);
    if (block->_prev) mergeBlockWithNext(block->_prev);
}

// block handling ------------------------------------------------------- //

// find block with enough free space, split it to size, and return it
MemMan::Block * MemMan::requestFreeBlock(size_t size) {
    size = ALIGN(size);

    Block * block = nullptr;
    for (block = _blockHead; block; block = block->_next) {
        if (block->_type == TYPE_FREE && block->dataSize() >= size) {
            break;
        }
    }
    if (!block) return nullptr;

    // We don't care if the split actually happens or not.
    // If it happened, we still return the first block and the 2nd is of no
    // concern.
    // If it didn't, it was the rare case where the block is big enough to
    // hold our data, but with not enough room to create a new block.
    splitBlock(block, size);

    // the old block is returned.
    // it was (probably) shrunk to fit by splitBlock.
    return block;
}

MemMan::Block * MemMan::splitBlock(Block * blockA, size_t blockANewSize) {
    // block A (this) is not big enough.
    // equal data size also rejected because new block would have
    // 0 bytes for data.
    if (blockA->dataSize() <= blockANewSize + BlockInfoSize) return nullptr;

    // where to put the new block?
    byte_t * newLoc = blockA->data() + blockANewSize;
    // write block info into data space with defaults
    Block * blockB = new (newLoc) Block();
    // block blockB gets remaining space
    blockB->_size = blockA->dataSize() - BlockInfoSize - blockANewSize;

    // set this size
    blockA->_size = blockANewSize;

    // link up
    blockB->_next = blockA->_next;
    blockB->_prev = blockA;
    blockA->_next = blockB;
    if (_blockTail == blockA) _blockTail = blockB;

    return blockA;
}

MemMan::Block * MemMan::splitBlockEnd(Block * blockA, size_t blockBNewSize) {
    if (blockBNewSize + BlockInfoSize > blockA->dataSize()) return nullptr;
    auto ret = splitBlock(blockA, blockA->dataSize() - BlockInfoSize - blockBNewSize);
    return (ret) ? ret->_next : nullptr;
}

MemMan::Block * MemMan::mergeBlockWithNext(Block * blockA) {
    if (blockA->_type != TYPE_FREE ||
        !blockA->_next ||
        blockA->_next->_type != TYPE_FREE)
        return nullptr;
    blockA->_size += blockA->_next->totalSize();
    blockA->_next = blockA->_next->_next;
    return blockA;
}

MemMan::Block * MemMan::resizeBlock(Block * block, size_t newSize) {
    // shrink block
    if (block->dataSize() >= newSize + BlockInfoSize) {
        return splitBlock(block, newSize);
    }
    // expand block
    else if (block->_next && block->_next->_type != TYPE_FREE) {
        Block * newBlock = nullptr;
        // room enough to expand in place (returns same block)
        if (block->dataSize() + block->_next->totalSize() <= newSize) {
            mergeBlockWithNext(block);
            return resizeBlock(block, newSize);
        }
        // copy to new place (frees old block, returns new block)
        else if ((newBlock = requestFreeBlock(newSize))) {
            memcpy(newBlock->data(), block->data(), block->dataSize());
            destroy(block->data());
            return newBlock;
        }
    }

    // failed
    return nullptr;
}

MemMan::Block const * MemMan::firstBlock() const {
    return _blockHead;
}

MemMan::Block const * MemMan::nextBlock(Block const & block) const {
    return block._next;
}


void MemMan::assertWithinData(void * ptr, size_t size) {
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

void * memManAlloc(size_t size, void * userData) {
    if (!size || !userData) return nullptr;
    MemMan * memMan = (MemMan *)userData;
    MemMan::Block * block = memMan->requestFreeBlock(size);
    if (!block) {
        fprintf(stderr, "Unable to alloc memory: %zu\n", size);
        assert(false);
    }
    block->_type = MemMan::TYPE_EXTERNAL;
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
    block = memMan->resizeBlock(block, size);
    return (block) ? block->data() : nullptr;
}

void memManFree(void * ptr, void * userData) {
    if (!ptr || !userData) return;
    MemMan * memMan = (MemMan *)userData;
    memMan->destroy(ptr);
}

void * BXAllocator::realloc(void * ptr, size_t size, size_t align, char const * file, uint32_t line) {
    assert(memMan && "Memory manager pointer not set in BXAllocator.");
    return memManRealloc(ptr, size, (void *)memMan);
}

// debug ---------------------------------------------------------------- //

void MemMan::getInfo(char * buf, int bufSize) {
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
