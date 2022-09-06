#pragma once
#include "GLTF.h"
#include "../common/string_utils.h"

struct GLTFSizeFinder {
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

    size_t accum = sizeof(gltf::GLTF);
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
        static constexpr size_t MaxShow = 50;
        if (length < MaxShow) {
            printl("%.*sString(%s, %zu, %d)", depth*4, PAD, str, length, copy);
        }
        else {
            printl("%.*sString(%.*s..., %zu, %d)", depth*4, PAD, MaxShow, str, length, copy);
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
            else if (strEqu(str, "bufferViews" )) { state = STATE_COUNT_BUFFERVIEWS; }
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
        using namespace gltf;
        if (depth == 2) {
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
        }

        --depth;

        printl("%.*sEndArray(%zu)", depth*4, PAD, elementCount);
        return true;
    }
};
