#include "MemMan.h"
#include <new>
#include "../common/file_utils.h"
#include "Pool.h"
#include "FrameStack.h"
#include "File.h"
#include "Gobj.h"
#include "FSA.h"
#include "File.h"
#include "FreeList.h"
#include "CharKeys.h"
#include "GLTFLoader4.h"

FrameStack * MemMan::createFrameStack(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({
        .size = sizeof(FrameStack) + size,
        .type = MEM_BLOCK_FRAMESTACK
    });
    if (!block) return nullptr;
    return new (block->data()) FrameStack{size};
}

File * MemMan::createFileHandle(char const * path, bool loadNow, bool high) {
    guard_t guard{_mainMutex};

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

    BlockInfo * block = createBlock({
        .size = size + sizeof(File),
        .type = MEM_BLOCK_FILE,
        .high = high
    });
    if (!block) return nullptr;
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

FreeList * MemMan::createFreeList(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({
        .size = sizeof(FreeList) + FreeList::DataSize(max),
        .type = MEM_BLOCK_FREELIST,
    });
    if (!block) return nullptr;
    return new (block->data()) FreeList{max};
}

CharKeys * MemMan::createCharKeys(size_t max) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({
        .size = sizeof(CharKeys) + CharKeys::DataSize(max),
        .type = MEM_BLOCK_CHARKEYS,
    });
    if (!block) return nullptr;
    return new (block->data()) CharKeys{max};
}

Gobj * MemMan::createGobj(char const * gltfPath) {
    guard_t guard{_mainMutex};

    // load gltf into high memory
    File * gltf = createFileHandle(gltfPath, true, false);
    if (gltf == nullptr) {
        fprintf(stderr, "Error creating File block\n");
        return nullptr;
    }

    // base path of gltf
    BlockInfo * pathBlock = createBlock({.size=strlen(gltfPath), .lifetime=0});
    char * basePath = (char *)pathBlock->data();
    getPath(gltfPath, basePath);

    // LOADER 4
    GLTFLoader4 loader4{gltf->data(), basePath};
    if (loader4.validData() == false) {
        fprintf(stderr, "Error creating Gobj block\n");
        return nullptr;
    }

    // calc size
    loader4.calculateSize();

    // create block
    BlockInfo * block = createBlock({
        .size = loader4.counts.totalSize(),
        .align = Gobj::Align,
        .type = MEM_BLOCK_GOBJ,
    });
    if (block == nullptr) {
        fprintf(stderr, "Error creating Gobj block\n");
        return nullptr;
    }
    Gobj * gobj = new (block->data()) Gobj{loader4.counts};

    // load into gobj
    bool success = loader4.load(gobj);

    // early return if load failed
    if (!success) {
        fprintf(stderr, "Error loading GLTF data into game object.\n");
        releaseBlock(block);
        return nullptr;
    }

    // check for exepected size
    #if DEBUG
    if (loader4.isGLB) {
        assert(
            gobj->buffer + alignSize(loader4.binDataSize(), Gobj::Align) ==
            block->data() + block->dataSize() &&
            "Gobj block unexpected size."
        );
    }
    #endif // DEBUG

    // free gltf file
    // request({.ptr=gltf, .size=0});

    // debug output
    #if DEBUG
    printl("LOADED GOBJ:");
    gobj->print();
    #endif // DEBUG

    return gobj;
}

void MemMan::createRequestResult() {
    guard_t guard{_mainMutex};

    // createBlock() depends on _request being set, so we set it temporarily
    Request tempRequest{
        .size = sizeof(Request) + sizeof(Result),
        .type = MEM_BLOCK_REQUEST
    };
    _request = &tempRequest;

    BlockInfo * block = createBlock();
    if (!block) return;
    _request = new (block->data()) Request{};
    _result = new (block->data()+sizeof(Request)) Result{};
}

FSA * MemMan::createFSA(MemManFSASetup const & setup) {
    guard_t guard{_mainMutex};

    // check to see if there are any FSA groups
    size_t dataSize = FSA::DataSize(setup);
    if (dataSize == 0) return nullptr;

    // solution to specifically align both FSA and containing block here, as it
    // makes size calculatable beforehand. otherwise differnce between FSA-base
    // and FSA-data might be unpredictable, resulting in
    size_t blockDataSize = alignSize(sizeof(FSA), setup.align) + dataSize;
    BlockInfo * block = createBlock({
        .size = blockDataSize,
        .align = setup.align,
        .type = MEM_BLOCK_FSA,
    });
    if (!block) return nullptr;

    // printl("FSA dataSize %zu", dataSize);
    // printl("sizeof(FSA) %zu", sizeof(FSA));
    // printl("alignSize sizeof(FSA) %zu", alignSize(sizeof(FSA), setup.align));
    // printl("FSA blockDataSize %zu", blockDataSize);
    // printl("aligned sizeof(FSA) %p", alignPtr(block->data() + sizeof(FSA), setup.align));

    FSA * fsa = new (block->data()) FSA{setup};

    // printl("FSA data (minus) base %zu", fsa->data() - block->data());
    return fsa;
}

Array<MemMan::AutoRelease> * MemMan::createAutoReleaseBuffer(size_t size) {
    guard_t guard{_mainMutex};

    if (size == 0) return nullptr;

    BlockInfo * block = createBlock({
        .size = sizeof(Array<AutoRelease>) + Array<AutoRelease>::DataSize(size),
        .align = 8,
        .type = MEM_BLOCK_ARRAY,
    });
    if (!block) return nullptr;

    Array<AutoRelease> * buf = new (block->data()) Array<AutoRelease>{size};

    return buf;
}
