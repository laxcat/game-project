#include "MemSys.h"
#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>
#include "../dev/print.h"
#include "GLTF.h"
#include "JSONPrinter.h"
#include "GLTFSizeFinder.h"

// using namespace rapidjson;

size_t MemSys::Entity::getMemorySize(FILE * externalFP) {
    using namespace rapidjson;

    JSONPrinter scanner;
    Reader reader;
    static constexpr size_t BufSize = 1024;
    char buf[BufSize];
    auto fs = FileReadStream(externalFP, buf, BufSize);
    reader.Parse(fs, scanner);


    return 999;
}

bool MemSys::Entity::readJSON(FILE * externalFP) {
    using namespace rapidjson;

    FILE * fp;

    // if file was already open, user might have passed in a file pointer
    // to save from reopening it
    if (externalFP) {
        fp = externalFP;
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
