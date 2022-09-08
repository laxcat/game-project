#include "MemSys.h"
#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>
#include "../common/code_timer.h"
#include "../dev/print.h"
#include "../MrManager.h"
#include "GLTF.h"
#include "JSONPrinter.h"
#include "GLTFSizeFinder.h"
#include "GLTFLoader2.h"

// using namespace rapidjson;

size_t MemSys::Entity::getMemorySize(FILE * externalFP) {
    code_timer_start();
    GLTFSizeFinder scanner;
    rapidjson::Reader reader;
    size_t bufSize = 1024*256;
    char * buf = (char *)mm.frameStack->alloc(bufSize);
    if (!buf) return 0;
    auto fs = rapidjson::FileReadStream(externalFP, buf, bufSize);
    reader.Parse(fs, scanner);
    // scanner.printStats();
    printl("read gltf size in %d µ", code_timer_end());
    return scanner.totalSize();
}

bool MemSys::Entity::load(FILE * externalFP, byte_t * dst, size_t dstSize) {
    _loaded = false;
    code_timer_start();
    GLTFLoader2 scanner;
    rapidjson::Reader reader;
    size_t bufSize = 1024*256;
    char * buf = (char *)mm.frameStack->alloc(bufSize);
    if (!buf) return 0;
    auto fs = rapidjson::FileReadStream(externalFP, buf, bufSize);
    reader.Parse(fs, scanner);
    printl("loaded gltf size in %d µ", code_timer_end());
    _loaded = true;
    return _loaded;
}
