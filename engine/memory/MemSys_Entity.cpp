#include "MemSys.h"
#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>
#include "../dev/print.h"

// using namespace rapidjson;

bool strEqu(char const * strA, char const * strB) {
    return (strcmp(strA, strB) == 0);
}

static constexpr char const * PAD = "                            ";
#define PVAL(TYPE, FORMAT, VAL) printl("%.*s" #TYPE "(" #FORMAT ")", depth*4, PAD, VAL)


struct Accessor {};
struct Animation {};
struct Asset {
    char const * copyright;
    char const * generator;
    char const * version;
    char const * minVersion;
};
struct Buffer {};
struct BufferView {};
struct Camera {};
struct Image {};
struct Material {};
struct Mesh {};
struct Node {};
struct Sampler {};
struct Scene {
    int * nodes;
    int nNodes;
    char const name[32];
};
struct Skin {};
struct Texture {};
struct GLTF {
    Accessor    * accessors;
    Animation   * animations;
    Asset         asset;
    Buffer      * buffers;
    BufferView  * bufferViews;
    Camera      * cameras;
    Image       * images;
    Material    * materials;
    Mesh        * meshes;
    Node        * nodes;
    Sampler     * samplers;
    Scene       * scenes;
    Skin        * skins;
    Texture     * textures;

    int16_t scene = -1;

    uint16_t nAccessors;
    uint16_t nAnimations;
    uint16_t nBuffers;
    uint16_t nBufferViews;
    uint16_t nCameras;
    uint16_t nImages;
    uint16_t nMaterials;
    uint16_t nMeshes;
    uint16_t nNodes;
    uint16_t nSamplers;
    uint16_t nScene;
    uint16_t nScenes;
    uint16_t nSkins;
    uint16_t nTextures;
};

struct SizeFinder {
    enum State {
        STATE_UNKNOWN,
        STATE_COUNT_ASSET_STRINGS,
        STATE_COUNT_ACCESSORS,
        STATE_COUNT_ANIMATIONS,
        STATE_COUNT_BUFFERS,
        STATE_COUNT_BUFFER_LENGTH,
        STATE_COUNT_BUFFERVIEWS,
        STATE_COUNT_CAMERAS,
        STATE_COUNT_IMAGES,
        STATE_COUNT_MATERIALS,
        STATE_COUNT_MESHES,
        STATE_COUNT_NODES,
        STATE_COUNT_SAMPLERS,
        STATE_COUNT_SCENES,
        STATE_COUNT_SKINS,
        STATE_COUNT_TEXTURES,
    };
    State state = STATE_UNKNOWN;

    size_t accum = sizeof(GLTF);
    size_t depth = 0;

    bool Null()             { PVAL(Null, %d, 0); return true; }
    bool Bool(bool b)       { PVAL(Bool, %d, b); return true; }
    bool Int(int i)         { PVAL(Int, %d, i); return true; }
    bool Uint(unsigned u)   {
        PVAL(Uint, %u, u);
        if (state == STATE_COUNT_BUFFER_LENGTH) { accum += u; state = STATE_COUNT_BUFFERS; }
        return true;
    }
    bool Int64(int64_t i)   { PVAL(Int64, %d, i); return true; }
    bool Uint64(uint64_t u) { PVAL(Uint64, %u, u); return true; }
    bool Double(double d)   { PVAL(Double, %f, d); return true; }
    bool RawNumber(char const * str, size_t length, bool copy) {
        printl("%.*sRawNumber(%s, %zu, %d)", depth*4, PAD, str, length, copy);
        return true;
    }
    bool String(char const * str, size_t length, bool copy) {
        if (length < 200) {
            printl("%.*sString(%s, %zu, %d)", depth*4, PAD, str, length, copy);
        }
        if (state == STATE_COUNT_ASSET_STRINGS) { accum += strlen(str); }
        return true;
    }
    bool StartObject() {
        printl("%.*sStartObject()", depth*4, PAD);
        ++depth;
        return true;
    }
    bool Key(char const * str, size_t length, bool copy) {
        if (depth == 1) {
            if      (strEqu(str, "asset"       )) { state = STATE_COUNT_ASSET_STRINGS; }
            else if (strEqu(str, "accessors"   )) { state = STATE_COUNT_ACCESSORS; }
            else if (strEqu(str, "animations"  )) { state = STATE_COUNT_ANIMATIONS; }
            else if (strEqu(str, "buffers"     )) { state = STATE_COUNT_BUFFERS; }
            else if (strEqu(str, "bufferviews" )) { state = STATE_COUNT_BUFFERVIEWS; }
            else if (strEqu(str, "cameras"     )) { state = STATE_COUNT_CAMERAS; }
            else if (strEqu(str, "images"      )) { state = STATE_COUNT_IMAGES; }
            else if (strEqu(str, "materials"   )) { state = STATE_COUNT_MATERIALS; }
            else if (strEqu(str, "meshes"      )) { state = STATE_COUNT_MESHES; }
            else if (strEqu(str, "nodes"       )) { state = STATE_COUNT_NODES; }
            else if (strEqu(str, "samplers"    )) { state = STATE_COUNT_SAMPLERS; }
            else if (strEqu(str, "scenes"      )) { state = STATE_COUNT_SCENES; }
            else if (strEqu(str, "skins"       )) { state = STATE_COUNT_SKINS; }
            else if (strEqu(str, "textures"    )) { state = STATE_COUNT_TEXTURES; }
        }
        else if (depth == 3) {
            if (state == STATE_COUNT_BUFFERS && strEqu(str, "byteLength")) { state = STATE_COUNT_BUFFER_LENGTH; }
        }
        // if (strEqu(str, "uri")) {
        //     skip = true;
        // }
        printl("%.*sKey(%s, %zu, %d)", depth*4, PAD, str, length, copy);
        return true;
    }
    bool EndObject(size_t memberCount) {
        if (state == STATE_COUNT_ASSET_STRINGS) state = STATE_UNKNOWN;
        --depth;
        printl("%.*sEndObject(%zu)", depth*4, PAD, memberCount);
        return true;
    }
    bool StartArray() {
        printl("%.*sStartArray()", depth*4, PAD);
        ++depth;
        return true;
    }
    bool EndArray(size_t elementCount) {
        if (state == STATE_COUNT_ACCESSORS) {
            printl("counted %zu Accessors", elementCount);
            accum += sizeof(Accessor) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_ANIMATIONS) {
            printl("counted %zu Animations", elementCount);
            accum += sizeof(Animation) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_BUFFERS) {
            printl("counted %zu Buffers", elementCount);
            accum += sizeof(Buffer) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_BUFFERVIEWS) {
            printl("counted %zu BufferViews", elementCount);
            accum += sizeof(BufferView) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_CAMERAS) {
            printl("counted %zu Cameras", elementCount);
            accum += sizeof(Camera) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_IMAGES) {
            printl("counted %zu Images", elementCount);
            accum += sizeof(Image) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_MATERIALS) {
            printl("counted %zu Materials", elementCount);
            accum += sizeof(Material) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_MESHES) {
            printl("counted %zu Meshes", elementCount);
            accum += sizeof(Mesh) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_NODES) {
            printl("counted %zu Nodes", elementCount);
            accum += sizeof(Node) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_SAMPLERS ) {
            printl("counted %zu Samplers", elementCount);
            accum += sizeof(Sampler) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_SCENES) {
            printl("counted %zu Scenes", elementCount);
            accum += sizeof(Scene) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_SKINS    ) {
            printl("counted %zu Skins", elementCount);
            accum += sizeof(Skin) * elementCount;
            state = STATE_UNKNOWN;
        }
        else if (state == STATE_COUNT_TEXTURES ) {
            printl("counted %zu Textures", elementCount);
            accum += sizeof(Texture) * elementCount;
            state = STATE_UNKNOWN;
        }

        --depth;

        printl("%.*sEndArray(%zu)", depth*4, PAD, elementCount);
        return true;
    }
};

size_t MemSys::Entity::getMemorySize(FILE * externalFP) {
    using namespace rapidjson;

    SizeFinder sizeFinder;
    printl("size of accum at init %zu", sizeFinder.accum);
    Reader reader;
    static constexpr size_t BufSize = 1024;
    char buf[BufSize];
    auto fs = FileReadStream(externalFP, buf, BufSize);
    reader.Parse(fs, sizeFinder);


    return sizeFinder.accum;
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
