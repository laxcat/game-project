#include "MemSys.h"
#include "../common/file_utils.h"
#include "../dev/print.h"

// lifecycle ------------------------------------------------------------ //

void MemSys::init(size_t size) {
    _data = (byte_t *)malloc(size);
    _size = size;
    _blockHead = new (_data) Block();
    _blockHead->size = size - BlockInfoSize;
}

void MemSys::shutdown() {
    if (_data) free(_data), _data = nullptr;
}

// create/destroy for internal types ------------------------------------ //

MemSys::Pool * MemSys::createPool(size_t objCount, size_t objSize) {
    Block * block = requestFreeBlock(objCount * objSize + sizeof(Pool));
    if (!block) return nullptr;
    block->type = TYPE_POOL;
    return new (block->data()) Pool{objCount, objSize};
}

void MemSys::destroyPool(Pool * a) {
    destroy(a);
}

Stack * MemSys::createStack(size_t size) {
    Block * block = requestFreeBlock(size + sizeof(Stack));
    if (!block) return nullptr;
    block->type = TYPE_STACK;
    return new (block->data()) Stack{size};
}

void MemSys::destroyStack(Stack * s) {
    destroy(s);
}

MemSys::File * MemSys::createFileHandle(char const * path, bool loadNow) {
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

void MemSys::destroyFileHandle(File * f) {
    destroy(f);
}

MemSys::Entity * MemSys::createEntity(char const * path, bool loadNow) {
    // open to deternmine size, and also maybe load
    errno = 0;
    FILE * fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error loading file \"%s\": %d\n", path, errno);
        return nullptr;
    }

    // calc size
    auto counter = Entity::getMemorySize(fp);
    size_t gltfSize = counter.totalSize();
    printl("gltf %s memory size %zu", path, gltfSize);

    // create block from size
    Block * block = requestFreeBlock(gltfSize + sizeof(Entity));
    if (!block) return nullptr;
    block->type = TYPE_ENTITY;
    Entity * entity = new (block->data()) Entity{path};

    // seek back to start of file
    int fseekError = fseek(fp, 0L, 0);
    if (fseekError) {
        fprintf(stderr, "Error seeking in file \"%s\": %d\n", path, fseekError);
        return nullptr;
    }

    // load it into memory
    entity->load(fp, entity->data(), gltfSize, counter);

    // close fp
    fclose(fp);

    entity->printGLTF();

    return nullptr;
}

void MemSys::destroyEntity(Entity * f) {
    destroy(f);
}

void MemSys::destroy(void * ptr) {
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
MemSys::Block * MemSys::requestFreeBlock(size_t size) {
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

bool MemSys::isWithinData(byte_t * ptr, size_t size) {
    return (ptr >= _data && ptr + size <= _data + _size);
}

// debug ---------------------------------------------------------------- //

void MemSys::getInfo(char * buf, int bufSize) {
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
