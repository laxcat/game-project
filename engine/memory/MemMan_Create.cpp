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
#include "GLTFLoader.h"
#if DEBUG
#include "../common/codetimer.h"
#endif // DEBUG

FrameStack * MemMan::createFrameStack(size_t size) {
    guard_t guard{_mainMutex};

    BlockInfo * block = createBlock({
        .size = sizeof(FrameStack) + size,
        .type = MEM_BLOCK_FRAMESTACK
    });
    if (!block) return nullptr;
    return new (block->data()) FrameStack{size};
}

File * MemMan::createFileHandle(char const * path, bool loadNow, Request const & addlRequest) {
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
        .size = size + alignSize(sizeof(File), addlRequest.align),
        .type = MEM_BLOCK_FILE,
        .high = addlRequest.high,
        .align = addlRequest.align,
        .lifetime = addlRequest.lifetime
    });
    if (!block) return nullptr;
    File * f = new (block->data()) File{size, path, addlRequest.align};

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

Gobj * MemMan::createGobj(char const * gltfPath, Gobj::Counts additionalCounts) {
    guard_t guard{_mainMutex};

    #if DEBUG
    codetimer::start();
    #endif // DEBUG

    // load gltf into high memory
    File * gltf = createFileHandle(
        gltfPath,
        true, // load now
        {
            .align = 4,
            .high = true,
            .lifetime = 0
        }
    );
    if (gltf == nullptr) {
        fprintf(stderr, "Error creating File block\n");
        return nullptr;
    }

    // base path of gltf
    uint32_t gltfPathLen = strlen(gltfPath);
    BlockInfo * dirNameBlock = createBlock({
        .size = gltfPathLen,
        .lifetime = 0,
        .high = true
    });
    char * dirName = (char *)dirNameBlock->data();
    uint32_t dirNameLen = (uint32_t)copyDirName(dirName, gltfPath);

    // LOADER
    BlockInfo * loaderBlock = createBlock({
        .size = sizeof(GLTFLoader),
        .lifetime = 0,
        .high = true
    });
    GLTFLoader * loader = new (loaderBlock->data()) GLTFLoader{gltf->data(), dirName};
    if (loader->validData() == false) {
        fprintf(stderr, "Error creating loader block\n");
        return nullptr;
    }

    // create room to save the loadedDirName string
    loader->setCounts({
        .allStrLen = dirNameLen + 1,
    });

    // calc size
    loader->calculateSize();

    // create gobj block
    Gobj * gobj = createGobj(loader->counts() + additionalCounts);
    BlockInfo * block = blockForPtr(gobj);
    if (block == nullptr) {
        fprintf(stderr, "Error getting Gobj block.\n");
        return nullptr;
    }

    // load into gobj
    gobj->setStatus(Gobj::STATUS_LOADING);
    bool success = loader->load(gobj);

    // early return if load failed
    if (!success) {
        gobj->setStatus(Gobj::STATUS_ERROR);
        fprintf(stderr, "Error loading GLTF data into game object.\n");
        releaseBlock(block);
        return nullptr;
    }

    // set loadedDirName
    gobj->loadedDirName = gobj->strings->copyStr(dirName, dirNameLen);

    // bytesLeft should be 0 at this point.
    assert(gobj->strings->bytesLeft() == 0 && "Miscalculation in string buffer. Should be full.");

    gobj->setStatus(Gobj::STATUS_LOADED);

    // check for exepected size
    #if DEBUG
    if (loader->isBinary()) {
        assert(
            gobj->rawData + alignSize(loader->binDataSize(), Gobj::Align) ==
            block->data() + block->dataSize() &&
            "Gobj block unexpected size."
        );
    }
    #endif // DEBUG

    // debug output
    #if DEBUG
    printl("LOADED GOBJ:");
    // printl(gobj->jsonStr);
    gobj->print();
    printl("Time to load from disk: %ldµs", codetimer::delta());
    #endif // DEBUG

    return gobj;
}

Gobj * MemMan::createGobj(Gobj::Counts const & counts, int lifetime) {
    // create block
    BlockInfo * block = createBlock({
        .size = counts.totalSize(),
        .align = Gobj::Align,
        .type = MEM_BLOCK_GOBJ,
        .lifetime = lifetime,
    });
    if (block == nullptr) {
        fprintf(stderr, "Error creating Gobj block\n");
        return nullptr;
    }
    return new (block->data()) Gobj{counts};
}

Gobj * MemMan::updateGobj(Gobj * oldGobj, Gobj::Counts additionalCounts) {
    Gobj * newGobj = createGobj(oldGobj->counts + additionalCounts);
    newGobj->copy(oldGobj);
    request({.ptr=oldGobj, .size=0});
    return newGobj;
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
