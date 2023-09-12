#include "GLTFLoader4.h"
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include "mem_utils.h"
#include "../common/file_utils.h"
#include "../common/Number.h"
#include "../common/modp_b64.h"
#include "../common/string_utils.h"
#include "../MrManager.h"

// -------------------------------------------------------------------------- //
// COUNTER
// -------------------------------------------------------------------------- //

GLTFLoader4::Counter::Counter(GLTFLoader4 * loader) :
    l(loader)
{
    #if DEBUG
    // add length of pretty JSON string to total string length
    l->prettyJSON(&l->counts.allStrLen);
    #endif // DEBUG
}

bool GLTFLoader4::Counter::Null() { return true; }
bool GLTFLoader4::Counter::Bool(bool b) { return true; }
bool GLTFLoader4::Counter::Int(int i) { return true; }
bool GLTFLoader4::Counter::Int64(int64_t i) { return true; }
bool GLTFLoader4::Counter::Uint64(uint64_t i) { return true; }
bool GLTFLoader4::Counter::Double(double d) { return true; }
bool GLTFLoader4::Counter::RawNumber(char const * str, uint32_t length, bool copy) { return true; }

bool GLTFLoader4::Counter::Uint(unsigned i) {
    if (l->crumb(-1).matches(TYPE_ARR, "buffers") &&
        strEqu(l->key, "byteLength")
    ) {
        l->counts.rawDataLen += i;
    }
    return true;
}

bool GLTFLoader4::Counter::String(char const * str, uint32_t length, bool copy) {
    // name strings
    if (l->crumb(-1).matches(TYPE_ARR,
            "accessors|animations|buffers|bufferViews|cameras|images|materials|"
            "meshes|nodes|samplers|scenes|skins|textures") &&
        strEqu(l->key, "name")
    ) {
        l->counts.allStrLen += length + 1;
    }

    // asset strings
    else if (
        l->crumb().matches(TYPE_OBJ, "asset") &&
        strWithin(l->key, "copyright|generator|version|minVersion")
    ) {
        l->counts.allStrLen += length + 1;
    }
    return true;
}


bool GLTFLoader4::Counter::StartObject() {
    l->push(TYPE_OBJ);
    return true;
}

bool GLTFLoader4::Counter::Key(char const * str, uint32_t length, bool copy) {
    snprintf(l->key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader4::Counter::EndObject(uint32_t memberCount) {
    if (l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches("primitives") &&
        l->crumb().matches("attributes"))
    {
        l->counts.meshAttributes += memberCount;
    }
    else if (l->crumb(-5).matches(TYPE_ARR, "meshes") &&
        l->crumb(-3).matches(TYPE_ARR, "primitives") &&
        l->crumb(-1).matches(TYPE_ARR, "targets"))
    {
        l->counts.meshAttributes += memberCount;
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Counter::StartArray() {
    l->push(TYPE_ARR);
    return true;
}

bool GLTFLoader4::Counter::EndArray(uint32_t elementCount) {
    Crumb & crumb = l->crumb();
    // count the main sub-objects
    if (l->depth == 2) {
        if      (crumb.matches("accessors"  )) { l->counts.accessors   = elementCount; }
        else if (crumb.matches("animations" )) { l->counts.animations  = elementCount; }
        else if (crumb.matches("buffers"    )) { l->counts.buffers     = elementCount; }
        else if (crumb.matches("bufferViews")) { l->counts.bufferViews = elementCount; }
        else if (crumb.matches("cameras"    )) { l->counts.cameras     = elementCount; }
        else if (crumb.matches("images"     )) { l->counts.images      = elementCount; }
        else if (crumb.matches("materials"  )) { l->counts.materials   = elementCount; }
        else if (crumb.matches("meshes"     )) { l->counts.meshes      = elementCount; }
        else if (crumb.matches("nodes"      )) { l->counts.nodes       = elementCount; }
        else if (crumb.matches("samplers"   )) { l->counts.samplers    = elementCount; }
        else if (crumb.matches("scenes"     )) { l->counts.scenes      = elementCount; }
        else if (crumb.matches("skins"      )) { l->counts.skins       = elementCount; }
        else if (crumb.matches("textures"   )) { l->counts.textures    = elementCount; }
    }
    // scene nodes
    else if (l->depth == 4 && crumb.matches("nodes") && l->crumb(-2).matches("scenes")) {
        l->counts.sceneNodes += elementCount;
    }
    // animation channels and samplers
    else if (l->depth == 4 && l->crumb(-2).matches("animations")) {
        if      (crumb.matches("channels")) { l->counts.animationChannels += elementCount; }
        else if (crumb.matches("samplers")) { l->counts.animationSamplers += elementCount; }
    }
    // mesh primitives and weights
    else if (l->depth == 4 && l->crumb(-2).matches("meshes")) {
        if      (crumb.matches("primitives")) { l->counts.meshPrimitives += elementCount; }
        else if (crumb.matches("weights")) { l->counts.meshWeights += elementCount; }
    }
    l->pop();
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //

GLTFLoader4::Scanner::Scanner(GLTFLoader4 * loader, Gobj * gobj) :
    l(loader),
    g(gobj)
{
    nextRawDataPtr = g->rawData;
    l->nextRawDataPtr = g->rawData;

    #if DEBUG
    // write pretty JSON into string buffer
    uint32_t length;
    char const * str = l->prettyJSON(&length);
    g->jsonStr = g->strings->writeStr(str, length);
    #endif // DEBUG
}

bool GLTFLoader4::Scanner::Null() { return false; }
bool GLTFLoader4::Scanner::Int(int i) { return false; }
bool GLTFLoader4::Scanner::Int64(int64_t i) { return false; }
bool GLTFLoader4::Scanner::Uint64(uint64_t i) { return false; }
bool GLTFLoader4::Scanner::Uint(unsigned i) { return false; }
bool GLTFLoader4::Scanner::Double(double d) { return false; }

bool GLTFLoader4::Scanner::Bool(bool b) {
    return (b) ?
        RawNumber("1", 1, false) :
        RawNumber("0", 1, false);
}

bool GLTFLoader4::Scanner::RawNumber(char const * str, uint32_t length, bool copy) {
    l->push(TYPE_NUM);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        l->crumb(-1).handleChild(l, g, str, length);
    }

    l->printBreadcrumbs();
    printl("%.*s", length, str);

    l->pop();
    return true;

    Number n{str, length};

    if (false) {}
    // if (l->crumb(-3).matches(TYPE_ARR, "accessors")) {
    //     uint16_t accIndex = l->crumb(-2).index;
    //     uint16_t minMaxIndex = l->crumb().index;
    //     if      (l->crumb(-1).matches("min")) { g->accessors[accIndex].min[minMaxIndex] = n; }
    //     else if (l->crumb(-1).matches("max")) { g->accessors[accIndex].max[minMaxIndex] = n; }
    // }
    else if (l->crumb(-3).matches(TYPE_ARR, "cameras")) {
        uint16_t camIndex = l->crumb(-2).index;
        Gobj::Camera * c = g->cameras + camIndex;
        char const * key = l->crumb().key;

        // unsafely relies on object member order. see below.
        if      (strWithin(key, "xmag,aspectRatio")) { c->_data[0] = n; }
        else if (strWithin(key, "ymag,yfov"))        { c->_data[1] = n; }
        else if (strEqu(key, "zfar"))                { c->_data[2] = n; }
        else if (strEqu(key, "znear"))               { c->_data[3] = n; }

        // since the above code unsafely relies on object member order, lets put
        // some static checks to make sure the members are where we expect them.
        static_assert(sizeof(float) == 4);
        static_assert(offsetof(Gobj::CameraOrthographic, xmag) == 0);
        static_assert(offsetof(Gobj::CameraOrthographic, ymag) == 4);
        static_assert(offsetof(Gobj::CameraOrthographic, zfar) == 8);
        static_assert(offsetof(Gobj::CameraOrthographic, znear) == 12);
        static_assert(offsetof(Gobj::CameraPerspective, aspectRatio) == 0);
        static_assert(offsetof(Gobj::CameraPerspective, yfov) == 4);
        static_assert(offsetof(Gobj::CameraPerspective, zfar) == 8);
        static_assert(offsetof(Gobj::CameraPerspective, znear) == 12);
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "materials") &&
        l->crumb().matches("doubleSided")) {
        uint16_t matIndex = l->crumb(-1).index;
        g->materials[matIndex].doubleSided = n;
    }
    // else if (l->crumb(-2).matches(TYPE_ARR, "accessors")) {
    //     uint16_t index = l->crumb(-1).index;
    //     if      (l->crumb().matches("bufferView"))     { g->accessors[index].bufferView = g->bufferViews + n; }
    //     else if (l->crumb().matches("byteOffset"))     { g->accessors[index].byteOffset = n; }
    //     else if (l->crumb().matches("componentType"))  { g->accessors[index].componentType = (Gobj::Accessor::ComponentType)(int)n; }
    //     else if (l->crumb().matches("normalized"))     { g->accessors[index].normalized = n; }
    //     else if (l->crumb().matches("count"))          { g->accessors[index].count = n; }

    // }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "animations") &&
        l->crumb(-2).matches(TYPE_ARR, "channels") &&
        l->crumb().matches("sampler"))
    {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-1).index;
        ac->sampler = g->animations[aniIndex].samplers + n;
    }
    else if (
        l->crumb(-5).matches(TYPE_ARR, "animations") &&
        l->crumb(-3).matches(TYPE_ARR, "channels") &&
        l->crumb(-1).matches(TYPE_OBJ, "target") &&
        l->crumb().matches("node"))
    {
        uint16_t aniIndex = l->crumb(-4).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-2).index;
        ac->node = g->nodes + n;
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "animations") &&
        l->crumb(-2).matches(TYPE_ARR, "samplers"))
    {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationSampler * as = g->animations[aniIndex].samplers + l->crumb(-1).index;
        if      (l->crumb().matches("input"))  { as->input  = g->accessors + n; }
        else if (l->crumb().matches("output")) { as->output = g->accessors + n; }
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "buffers") &&
        l->crumb().matches("byteLength"))
    {
        uint16_t bufIndex = l->crumb(-1).index;
        g->buffers[bufIndex].byteLength = n;
        if (l->isGLB && bufIndex == 0) {
            if ((uint32_t)n != l->binDataSize()) {
                fprintf(stderr, "Unexpected buffer size.\n");
                return false;
            }
            memcpy(nextRawDataPtr, l->binData(), n);
            g->buffers[bufIndex].data = nextRawDataPtr;
            nextRawDataPtr += n;
        }
    }
    else if (l->crumb(-2).matches(TYPE_ARR, "bufferViews")) {
        uint16_t bvIndex = l->crumb(-1).index;
        Gobj::BufferView * bv = g->bufferViews + bvIndex;
        if      (l->crumb().matches("buffer"))     { bv->buffer = g->buffers + n; }
        else if (l->crumb().matches("byteLength")) { bv->byteLength = n; }
        else if (l->crumb().matches("byteOffset")) { bv->byteOffset = n; }
        else if (l->crumb().matches("byteStride")) { bv->byteStride = n; }
        else if (l->crumb().matches("target"))     { bv->target = (Gobj::BufferView::Target)(int)n; }
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "images") &&
        l->crumb().matches("bufferView"))
    {
        uint16_t imgIndex = l->crumb(-1).index;
        Gobj::Image * img = g->images + imgIndex;
        img->bufferView = g->bufferViews + n;
    }
    else if (l->crumb(-4).matches(TYPE_ARR, "materials")) {
        uint16_t matIndex = l->crumb(-3).index;
        Gobj::Material * mat = g->materials + matIndex;
        if (l->crumb().matches("index")) {
            if      (l->crumb(-1).matches("baseColorTexture"))        { mat->baseColorTexture = g->textures + n; }
            else if (l->crumb(-1).matches("metallicRoughnessTexture")){ mat->metallicRoughnessTexture = g->textures + n; }
        }
        else if (l->crumb().matches("texCoord")) {
            if      (l->crumb(-1).matches("baseColorTexture"))        { mat->baseColorTexCoord = (Gobj::Attr)(int)n; }
            else if (l->crumb(-1).matches("metallicRoughnessTexture")){ mat->metallicRoughnessTexCoord = (Gobj::Attr)(int)n; }
        }
        else {
            handleMaterialData(n);
        }
    }
    else if (l->crumb(-3).matches(TYPE_ARR, "materials")) {
        uint16_t matIndex = l->crumb(-2).index;
        Gobj::Material * mat = g->materials + matIndex;
        if (l->crumb().matches("index")) {
            if      (l->crumb(-1).matches("emissiveTexture"))   { mat->emissiveTexture = g->textures + n; }
            else if (l->crumb(-1).matches("normalTexture"))     { mat->normalTexture = g->textures + n; }
            else if (l->crumb(-1).matches("occlusionTexture"))  { mat->occlusionTexture = g->textures + n; }
        }
        else if (l->crumb().matches("texCoord")) {
            if      (l->crumb(-1).matches("emissiveTexture"))   { mat->emissiveTexCoord = (Gobj::Attr)(int)n; }
            else if (l->crumb(-1).matches("normalTexture"))     { mat->normalTexCoord = (Gobj::Attr)(int)n; }
            else if (l->crumb(-1).matches("occlusionTexture"))  { mat->occlusionTexCoord = (Gobj::Attr)(int)n; }
        }
        else {
            handleMaterialData(n);
        }
    }
    else if (
        (l->crumb(-5).matches(TYPE_ARR, "meshes") &&
        l->crumb(-3).matches(TYPE_ARR, "primitives") &&
        l->crumb(-1).matches("attributes"))
        ||
        (l->crumb(-6).matches(TYPE_ARR, "meshes") &&
        l->crumb(-4).matches(TYPE_ARR, "primitives") &&
        l->crumb(-2).matches(TYPE_ARR, "targets")))
    {
        g->meshAttributes[nextMeshAttribute].type = l->attrFromStr(l->key);
        g->meshAttributes[nextMeshAttribute].accessor = g->accessors + n;
        ++nextMeshAttribute;
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches(TYPE_ARR, "primitives"))
    {
        uint16_t mshIndex = l->crumb(-3).index;
        uint16_t priIndex = l->crumb(-1).index;
        Gobj::MeshPrimitive * prim = &g->meshes[mshIndex].primitives[priIndex];
        if      (l->crumb().matches("indices"))  { prim->indices = g->accessors + n; }
        else if (l->crumb().matches("material")) { prim->material = g->materials + n; }
        else if (l->crumb().matches("mode"))     { prim->mode = (Gobj::MeshPrimitive::Mode)(int)n; }
    }
    else if (
        l->crumb(-3).matches(TYPE_ARR, "meshes") &&
        l->crumb(-1).matches(TYPE_ARR, "weights"))
    {
        uint16_t mshIndex = l->crumb(-2).index;
        uint16_t numIndex = l->crumb().index;
        g->meshes[mshIndex].weights[numIndex] = n;
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "materials") &&
        l->crumb().matches("alphaCutoff"))
    {
        uint16_t matIndex = l->crumb(-1).index;
        g->materials[matIndex].alphaCutoff = n;
    }

    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::String(char const * str, uint32_t length, bool copy) {
    l->push(TYPE_STR);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        l->crumb(-1).handleChild(l, g, str, length);
    }

    l->printBreadcrumbs();
    printl("\"%s\"", str);

    l->pop();
    return true;

    // handle name strings
    if (l->crumb().matches("name")) {
        char const * key = l->crumb(-2).key;
        uint16_t index = l->crumb(-1).index;
        if (false) {}
        // if      (strEqu(key, "accessors"))  { g->accessors[index].name   = g->strings->writeStr(str, length); }
        else if (strEqu(key, "animations")) { g->animations[index].name  = g->strings->writeStr(str, length); }
        else if (strEqu(key, "buffers"))    { g->buffers[index].name     = g->strings->writeStr(str, length); }
        else if (strEqu(key, "bufferViews")){ g->bufferViews[index].name = g->strings->writeStr(str, length); }
        else if (strEqu(key, "cameras"))    { g->cameras[index].name     = g->strings->writeStr(str, length); }
        else if (strEqu(key, "images"))     { g->images[index].name      = g->strings->writeStr(str, length); }
        else if (strEqu(key, "materials"))  { g->materials[index].name   = g->strings->writeStr(str, length); }
        else if (strEqu(key, "meshes"))     { g->meshes[index].name      = g->strings->writeStr(str, length); }
        else if (strEqu(key, "nodes"))      { g->nodes[index].name       = g->strings->writeStr(str, length); }
        else if (strEqu(key, "samplers"))   { g->samplers[index].name    = g->strings->writeStr(str, length); }
        else if (strEqu(key, "scenes"))     { g->scenes[index].name      = g->strings->writeStr(str, length); }
        else if (strEqu(key, "skins"))      { g->skins[index].name       = g->strings->writeStr(str, length); }
        else if (strEqu(key, "textures"))   { g->textures[index].name    = g->strings->writeStr(str, length); }
    }
    else if (l->crumb(-1).matches(TYPE_OBJ, "asset")) {
        // if      (l->crumb().matches("copyright"))  { g->copyright   = g->strings->writeStr(str, length); }
        // else if (l->crumb().matches("generator"))  { g->generator   = g->strings->writeStr(str, length); }
        // else if (l->crumb().matches("version"))    { g->version     = g->strings->writeStr(str, length); }
        // else if (l->crumb().matches("minVersion")) { g->minVersion  = g->strings->writeStr(str, length); }
    }
    // else if (l->crumb(-2).matches(TYPE_ARR, "accessors")) {
    //     uint16_t index = l->crumb(-1).index;
    //     if     (l->crumb().matches("type")) { g->accessors[index].type = l->accessorTypeFromStr(str); }

    // }
    else if (
        l->crumb(-5).matches(TYPE_ARR, "animations") &&
        l->crumb(-3).matches(TYPE_ARR, "channels") &&
        l->crumb(-1).matches(TYPE_OBJ, "target") &&
        l->crumb().matches("path"))
    {
        uint16_t aniIndex = l->crumb(-4).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-2).index;
        ac->path = l->animationTargetFromStr(str);
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "animations") &&
        l->crumb(-2).matches(TYPE_ARR, "samplers") &&
        l->crumb().matches("interpolation"))
    {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationSampler * as = g->animations[aniIndex].samplers + l->crumb(-1).index;
        as->interpolation = l->interpolationFromStr(str);
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "buffers") &&
        l->crumb().matches("uri"))
    {
            uint16_t bufIndex = l->crumb(-1).index;
            if (l->isGLB) {
                fprintf(stderr, "Unexpected buffer.uri in GLB.\n");
                return false;
            }
            size_t bytesWritten = l->handleData(nextRawDataPtr, str, length);
            if (bytesWritten == 0) {
                fprintf(stderr, "No bytes written in buffer %d.\n", bufIndex);
                return false;
            }
            Gobj::Buffer * buf = g->buffers + bufIndex;
            buf->data = nextRawDataPtr;
            buf->byteLength = bytesWritten;
            nextRawDataPtr += bytesWritten;
    }
    else if (l->crumb(-2).matches("cameras") && l->crumb().matches("type")) {
        Gobj::Camera * c = g->cameras + l->crumb(-1).index;
        c->type = l->cameraTypeFromStr(str);
    }
    else if (l->crumb(-2).matches(TYPE_ARR, "images")) {
        if (l->crumb().matches("uri")) {
            fprintf(stderr, "WARNING, external images are not loaded to main memory. TODO: load directly to GPU.\n");
        }
        else if (l->crumb().matches("mimeType")) {
            uint16_t imgIndex = l->crumb(-1).index;
            Gobj::Image * img = g->images + imgIndex;
            img->mimeType = l->imageMIMETypeFromStr(str);
        }
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "materials") &&
        l->crumb().matches("alphaMode")) {
        uint16_t matIndex = l->crumb(-1).index;
        g->materials[matIndex].alphaMode = l->alphaModeFromStr(str);
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::StartObject() {
    l->push(TYPE_OBJ);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        l->crumb(-1).handleChild(l, g, nullptr, 0);
    }
    // set new handle child
    if (l->depth == 1) {
        l->crumb().handleChild = handleRoot;
    }

    l->printBreadcrumbs();
    printl();

    if (l->crumb(-1).matches(TYPE_ARR, "animations")) {
        uint16_t index = l->crumb().index;
        g->animations[index].channels = g->animationChannels + nextAnimationChannel;
        g->animations[index].samplers = g->animationSamplers + nextAnimationSampler;
    }
    else if (l->crumb(-2).matches(TYPE_ARR, "cameras")) {
        uint16_t camIndex = l->crumb(-1).index;
        Gobj::Camera * c = g->cameras + camIndex;
        if (l->crumb().matches("orthographic")) {
            assert(c->perspective == nullptr && "Orthographic camera found but perspective already set.");
            c->orthographic = (Gobj::CameraOrthographic *)&c->_data;
        }
        else if (l->crumb().matches("perspective")) {
            assert(c->orthographic == nullptr && "Orthographic camera found but orthographic already set.");
            c->perspective = (Gobj::CameraPerspective *)&c->_data;
        }
    }
    else if (l->crumb(-1).matches(TYPE_ARR, "meshes")) {
        uint16_t index = l->crumb().index;
        g->meshes[index].primitives = g->meshPrimitives + nextMeshPrimitive;
        g->meshes[index].weights = g->meshWeights + nextMeshWeight;
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches(TYPE_ARR, "primitives") &&
        l->crumb().matches("attributes"))
    {
        uint16_t mshIndex = l->crumb(-3).index;
        uint16_t priIndex = l->crumb(-1).index;
        g->meshes[mshIndex].primitives[priIndex].attributes = g->meshAttributes + nextMeshAttribute;
    }
    return true;
}

bool GLTFLoader4::Scanner::Key(char const * str, uint32_t length, bool copy) {
    snprintf(l->key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader4::Scanner::EndObject(uint32_t memberCount) {
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::StartArray() {
    l->push(TYPE_ARR);
    if (l->crumb(-1).handleChild) {
        l->crumb(-1).handleChild(l, g, nullptr, 0);
    }
    l->printBreadcrumbs();
    printl();
    return true;
}

bool GLTFLoader4::Scanner::EndArray(uint32_t elementCount) {
    // if (l->crumb(-2).matches(TYPE_ARR, "animations")) {
    //     uint16_t aniIndex = l->crumb(-1).index;
    //     if (l->crumb().matches("channels")) {
    //         g->animations[aniIndex].nChannels = elementCount;
    //         nextAnimationChannel += elementCount;
    //     }
    //     else if (l->crumb().matches("samplers")) {
    //         g->animations[aniIndex].nSamplers = elementCount;
    //         nextAnimationSampler += elementCount;
    //     }
    // }
    if (l->crumb().handleEnd) {
        l->crumb().handleEnd(l, g, elementCount);
    }
    l->pop();
    return true;
}

void GLTFLoader4::Scanner::handleMaterialData(float num) {
    uint16_t matIndex = l->crumb(-2).index;
    if (l->crumb(-1).matches(TYPE_ARR, "emissiveFactor")) {
        uint16_t efIndex = l->crumb().index;
        g->materials[matIndex].emissiveFactor[efIndex] = num;
    }
    if (l->crumb(-1).matches(TYPE_ARR, "baseColorFactor")) {
        uint16_t bcfIndex = l->crumb().index;
        g->materials[matIndex].baseColorFactor[bcfIndex] = num;
    }
    else if (l->crumb(-1).matches("normalTexture") && l->crumb().matches("scale")) {
        g->materials[matIndex].normalScale = num;
    }
    else if (l->crumb(-1).matches("occlusionTexture") && l->crumb().matches("strength")) {
        g->materials[matIndex].occlusionStrength = num;
    }
    else if (l->crumb(-1).matches("pbrMetallicRoughness")) {
        if (l->crumb().matches("metallicFactor")) { g->materials[matIndex].metallicFactor = num; }
        else if (l->crumb().matches("roughnessFactor")) { g->materials[matIndex].roughnessFactor = num; }
    }

}

// -------------------------------------------------------------------------- //
// CRUMB
// -------------------------------------------------------------------------- //

GLTFLoader4::Crumb::Crumb(ObjType objType, char const * key) {
    this->objType = objType;
    setKey(key);
}

GLTFLoader4::Crumb::Crumb() {}

void GLTFLoader4::Crumb::setKey(char const * key) {
    if (key) {
        fixstrcpy<MaxKeyLen>(this->key, key);
    }
    else {
        this->key[0] = '\0';
    }
}

bool GLTFLoader4::Crumb::matches(char const * key) const {
    if (objType == TYPE_UNKNOWN) return false;
    // TODO: unsure of how these key rules should work... simplify this
    assert(key);
    // passed null key matches any (no check, always true)
    if (!key || key[0] == '\0') return true;
    // always false if Crumb has no key, except true when empty string is passed
    if (!hasKey()) return strEqu(key, "");
    // both passed and Crumb key exists, check if passed key contains Crumb key
    return strWithin(this->key, key, '|');
}

bool GLTFLoader4::Crumb::matches(ObjType objType, char const * key) const {
    if (objType != this->objType) return false;
    return matches(key);
}

char const * GLTFLoader4::Crumb::objTypeStr() const {
    return GLTFLoader4::objTypeStr(objType);
}

bool GLTFLoader4::Crumb::hasKey() const { return (key[0] != '\0'); }

bool GLTFLoader4::Crumb::isValid() const { return (objType != TYPE_UNKNOWN); }

// -------------------------------------------------------------------------- //
// LOADER
// -------------------------------------------------------------------------- //

GLTFLoader4::GLTFLoader4(byte_t const * gltfData, char const * loadingDir) :
    gltfData(gltfData),
    loadingDir(loadingDir)
{
    if (strEqu((char const *)gltfData, "glTF", 4)) {
        isGLB = true;

        if (!strEqu((char const *)(gltfData + 16), "JSON", 4)) {
            fprintf(stderr, "glTF JSON chunk magic string not found.\n");
            gltfData = nullptr;
            return;
        }
        if (!strEqu((char const *)(binChunkStart() + 4), "BIN\0", 4)) {
            fprintf(stderr, "BIN chunk magic string not found.\n");
            gltfData = nullptr;
            return;
        }
    }
    else {
        gltfData = nullptr;
    }
}

GLTFLoader4::Crumb & GLTFLoader4::crumb(int offset) {
    // out of bounds, return invalid crumb
    if (depth <= -offset || offset > 0) {
        return invalidCrumb;
    }
    return crumbs[depth-1+offset];
}

void GLTFLoader4::calculateSize() {
    assert(gltfData && "GLTF data invalid.");
    Counter counter{this};
    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr());
    reader.Parse<rapidjson::kParseStopWhenDoneFlag>(ss, counter);
}

bool GLTFLoader4::load(Gobj * gobj) {
    using namespace rapidjson;
    assert(gltfData && "GLTF data invalid.");
    key[0] = '\0';
    Scanner scanner{this, gobj};
    Reader reader;
    auto ss = StringStream(jsonStr());
    ParseResult result =
        reader.Parse<kParseStopWhenDoneFlag|kParseNumbersAsStringsFlag>(ss, scanner);
    if (result.IsError()) {
        fprintf(stderr, "JSON parse error: %s (%zu)\n",
            GetParseError_En(result.Code()), result.Offset());
    }
    return result;
}

char const * GLTFLoader4::prettyJSON(uint32_t * prettyJSONSize) const {
    assert(gltfData && "GLTF data invalid.");

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr());
    reader.Parse(ss, writer);

    if (prettyJSONSize) {
        *prettyJSONSize = sb.GetSize() + 1;
    }
    return mm.frameFormatStr("%s", sb.GetString());
}

void GLTFLoader4::printBreadcrumbs() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        if (crumbs[i].index == -1) {
            if (crumbs[i].key[0]) {
                print("%s:", crumbs[i].key);
            }
            print("%s > ", crumbs[i].objTypeStr());
        }
        else {
            print("%d:%s > ", crumbs[i].index, crumbs[i].objTypeStr());
        }
    }
}

uint32_t GLTFLoader4::jsonStrSize() const {
    assert(isGLB && "jsonStrSize() not available when isGLB==false.");
    return *(uint32_t *)(gltfData + 12);
}

char const * GLTFLoader4::jsonStr() const {
    // returns gltfData if GLTF,
    // returns gltfData+20 if GLB
    return (char const *)(gltfData + (isGLB * 20));
}

byte_t const * GLTFLoader4::binChunkStart() const {
    assert(isGLB && "No binary chuck to access.");
    // aligned because GLTF spec will add space characters at end of json
    // string in order to align to 4-byte boundary
    return (byte_t const *)alignPtr((void *)(jsonStr() + jsonStrSize()), 4);
}

uint32_t GLTFLoader4::binDataSize() const {
    return *(uint32_t *)binChunkStart();
}

byte_t const * GLTFLoader4::binData() const {
    return binChunkStart() + 8;
}

bool GLTFLoader4::validData() const {
    return gltfData;
}

char const * GLTFLoader4::objTypeStr(ObjType objType) {
    return
        (objType == TYPE_OBJ)   ? "OBJ" :
        (objType == TYPE_ARR)   ? "ARR" :
        (objType == TYPE_STR)   ? "STR" :
        (objType == TYPE_NUM)   ? "NUM" :
        "UNKNOWN";
}

void GLTFLoader4::push(ObjType objType) {
    assert(depth < MaxDepth && "Can't push crumb.");

    // push new crumb
    ++depth;
    Crumb & newCrumb = crumb();
    Crumb & parentCrumb = crumb(-1);

    // set index
    // if parent is an array, mark the index in newly pushed child and increntment next index.
    if (parentCrumb.objType == TYPE_ARR) {
        newCrumb.index = parentCrumb.childCount;
        ++parentCrumb.childCount;
    }
    else {
        newCrumb.index = -1;
    }
    // set type
    newCrumb.objType = objType;
    // set key (copy and reset buffer)
    newCrumb.setKey(key);
    key[0] = '\0';
    // reset everthing
    newCrumb.childCount = 0;
    newCrumb.handleChild = nullptr;
}

void GLTFLoader4::pop() {
    --depth;
}

Gobj::Accessor::Type GLTFLoader4::accessorTypeFromStr(char const * str) {
    if (strEqu(str, "SCALAR")) return Gobj::Accessor::TYPE_SCALAR;
    if (strEqu(str, "VEC2"  )) return Gobj::Accessor::TYPE_VEC2;
    if (strEqu(str, "VEC3"  )) return Gobj::Accessor::TYPE_VEC3;
    if (strEqu(str, "VEC4"  )) return Gobj::Accessor::TYPE_VEC4;
    if (strEqu(str, "MAT2"  )) return Gobj::Accessor::TYPE_MAT2;
    if (strEqu(str, "MAT3"  )) return Gobj::Accessor::TYPE_MAT3;
    if (strEqu(str, "MAT4"  )) return Gobj::Accessor::TYPE_MAT4;
    return Gobj::Accessor::TYPE_UNDEFINED;
}

Gobj::AnimationTarget GLTFLoader4::animationTargetFromStr(char const * str) {
    if (strEqu(str, "weights"    )) return Gobj::ANIM_TAR_WEIGHTS;
    if (strEqu(str, "translation")) return Gobj::ANIM_TAR_TRANSLATION;
    if (strEqu(str, "rotation"   )) return Gobj::ANIM_TAR_ROTATION;
    if (strEqu(str, "scale"      )) return Gobj::ANIM_TAR_SCALE;
    return Gobj::ANIM_TAR_UNDEFINED;
}

Gobj::AnimationSampler::Interpolation GLTFLoader4::interpolationFromStr(char const * str) {
    if (strEqu(str, "STEP"       )) return Gobj::AnimationSampler::INTERP_STEP;
    if (strEqu(str, "CUBICSPLINE")) return Gobj::AnimationSampler::INTERP_CUBICSPLINE;
    return Gobj::AnimationSampler::INTERP_LINEAR;
}

Gobj::Camera::Type GLTFLoader4::cameraTypeFromStr(char const * str) {
    if (strEqu(str, "orthographic")) return Gobj::Camera::TYPE_ORTHO;
    if (strEqu(str, "perspective" )) return Gobj::Camera::TYPE_PERSP;
    return Gobj::Camera::TYPE_PERSP;
}

Gobj::Image::MIMEType GLTFLoader4::imageMIMETypeFromStr(char const * str) {
    if (strEqu(str, "image/jpeg"))  return Gobj::Image::TYPE_JPEG;
    if (strEqu(str, "image/png" ))  return Gobj::Image::TYPE_PNG;
    return Gobj::Image::TYPE_PNG;
}

Gobj::Material::AlphaMode GLTFLoader4::alphaModeFromStr(char const * str) {
    if (strEqu(str, "ALPHA_OPAQUE"))    return Gobj::Material::ALPHA_OPAQUE;
    if (strEqu(str, "ALPHA_MASK" ))     return Gobj::Material::ALPHA_MASK;
    if (strEqu(str, "ALPHA_BLEND" ))    return Gobj::Material::ALPHA_BLEND;
    return Gobj::Material::ALPHA_OPAQUE;
}

Gobj::Attr GLTFLoader4::attrFromStr(char const * str) {
    if (strEqu(str, "POSITION"))   return Gobj::ATTR_POSITION;
    if (strEqu(str, "NORMAL"))     return Gobj::ATTR_NORMAL;
    if (strEqu(str, "TANGENT"))    return Gobj::ATTR_TANGENT;
    if (strEqu(str, "BITANGENT"))  return Gobj::ATTR_BITANGENT;
    if (strEqu(str, "COLOR0"))     return Gobj::ATTR_COLOR0;
    if (strEqu(str, "COLOR1"))     return Gobj::ATTR_COLOR1;
    if (strEqu(str, "COLOR2"))     return Gobj::ATTR_COLOR2;
    if (strEqu(str, "COLOR3"))     return Gobj::ATTR_COLOR3;
    if (strEqu(str, "INDICES"))    return Gobj::ATTR_INDICES;
    if (strEqu(str, "WEIGHT"))     return Gobj::ATTR_WEIGHT;
    if (strEqu(str, "TEXCOORD_0")) return Gobj::ATTR_TEXCOORD0;
    if (strEqu(str, "TEXCOORD_1")) return Gobj::ATTR_TEXCOORD1;
    if (strEqu(str, "TEXCOORD_2")) return Gobj::ATTR_TEXCOORD2;
    if (strEqu(str, "TEXCOORD_3")) return Gobj::ATTR_TEXCOORD3;
    if (strEqu(str, "TEXCOORD_4")) return Gobj::ATTR_TEXCOORD4;
    if (strEqu(str, "TEXCOORD_5")) return Gobj::ATTR_TEXCOORD5;
    if (strEqu(str, "TEXCOORD_6")) return Gobj::ATTR_TEXCOORD6;
    if (strEqu(str, "TEXCOORD_7")) return Gobj::ATTR_TEXCOORD7;
    if (strEqu(str, "TEXCOORD0"))  return Gobj::ATTR_TEXCOORD0;
    if (strEqu(str, "TEXCOORD1"))  return Gobj::ATTR_TEXCOORD1;
    if (strEqu(str, "TEXCOORD2"))  return Gobj::ATTR_TEXCOORD2;
    if (strEqu(str, "TEXCOORD3"))  return Gobj::ATTR_TEXCOORD3;
    if (strEqu(str, "TEXCOORD4"))  return Gobj::ATTR_TEXCOORD4;
    if (strEqu(str, "TEXCOORD5"))  return Gobj::ATTR_TEXCOORD5;
    if (strEqu(str, "TEXCOORD6"))  return Gobj::ATTR_TEXCOORD6;
    if (strEqu(str, "TEXCOORD7"))  return Gobj::ATTR_TEXCOORD7;
    return Gobj::ATTR_POSITION;
}

size_t GLTFLoader4::handleData(byte_t * dst, char const * str, size_t strLength) {
    // base64?
    static char const * isDataStr = "data:application/octet-stream;base64,";
    size_t isDataLen = strlen(isDataStr);
    if (strEqu(str, isDataStr, isDataLen)) {
        size_t b64StrLen = strLength - isDataLen;
        modp_b64_decode((char *)dst, (char *)str + isDataLen, b64StrLen);
        return modp_b64_decode_len(b64StrLen);
    }

    // uri to load?
    // TODO: test this
    char * fullPath = mm.frameFormatStr("%s%s", loadingDir, str);
    FILE * fp = fopen(fullPath, "r");
    if (!fp) {
        fprintf(stderr, "WARNING: Error opening buffer file: %s\n", fullPath);
        return 0;
    }
    long fileSize = getFileSize(fp);
    if (fileSize == -1) {
        fclose(fp);
        return 0;
    }
    size_t readSize = fread(dst, 1, fileSize, fp);
    // error
    if (ferror(fp) || readSize != fileSize) {
        fprintf(stderr, "Error reading file \"%s\" contents: read %zu, expecting %zu\n",
            fullPath, readSize, fileSize);
        fclose(fp);
        return 0;
    }

    return readSize;
}

bool GLTFLoader4::handleRoot(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    switch(*(uint16_t *)c.key) {
    // accessors
    case 'a'|'c'<<8: {
        printl("!!!!!!!! accessors");
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAccessor;
        };
        break; }
    // asset
    case 'a'|'s'<<8: {
        printl("!!!!!!!! asset");
        c.handleChild = handleAsset;
        break; }
    // animations
    case 'a'|'n'<<8: {
        printl("!!!!!!!! animations");
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAnimation;
        };
        break; }
    // buffers|bufferViews
    case 'b'|'u'<<8: {
        switch(c.key[6]) {
        // buffers
        case 's': {
            printl("!!!!!!!! buffers");
            c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
                l->crumb().handleChild = handleBuffer;
            };
            break; }
        // bufferViews
        case 'V': {
            printl("!!!!!!!! bufferViews");
            c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
                l->crumb().handleChild = handleBufferView;
            };
            break; }
        }
        break; }
    // cameras
    case 'c'|'a'<<8: {
        printl("!!!!!!!! cameras");
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleCamera;
        };
        break; }
    // images
    case 'i'|'m'<<8: { printl("!!!!!!!! images"); break; }
    // materials
    case 'm'|'a'<<8: { printl("!!!!!!!! materials"); break; }
    // meshes
    case 'm'|'e'<<8: { printl("!!!!!!!! meshes"); break; }
    // nodes
    case 'n'|'o'<<8: { printl("!!!!!!!! nodes"); break; }
    // samplers
    case 's'|'a'<<8: { printl("!!!!!!!! samplers"); break; }
    // scenes
    case 's'|'c'<<8: { printl("!!!!!!!! scenes"); break; }
    // skins
    case 's'|'k'<<8: { printl("!!!!!!!! skins"); break; }
    // textures
    case 't'|'e'<<8: { printl("!!!!!!!! textures"); break; }
    }
    return true;
}

bool GLTFLoader4::handleAccessor(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Accessor * a = g->accessors + l->crumb(-1).index;
    Number n{str, len};

    switch(*(uint16_t *)c.key) {
    // bufferView
    case 'b'|'u'<<8: {
        a->bufferView = g->bufferViews + n;
        break; }
    // byteOffset
    case 'b'|'y'<<8: {
        a->byteOffset = n;
        break; }
    // componentType|count
    case 'c'|'o'<<8: {
        if (c.key[2]=='m') {
            a->componentType = (Gobj::Accessor::ComponentType)(int)n;
        }
        else {
            a->count = n;
        }
        break; }
    // max
    case 'm'|'a'<<8: {
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            uint16_t accIndex = l->crumb(-2).index;
            uint16_t maxIndex = l->crumb().index;
            g->accessors[accIndex].max[maxIndex] = Number{str, len};
        };
        break; }
    // min
    case 'm'|'i'<<8: {
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            uint16_t accIndex = l->crumb(-2).index;
            uint16_t minIndex = l->crumb().index;
            g->accessors[accIndex].min[minIndex] = Number{str, len};
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        a->name = g->strings->writeStr(str, len);
        break; }
    // normalized
    case 'n'|'o'<<8: {
        a->normalized = n;
        break; }
    // type
    case 't'|'y'<<8: {
        a->type = l->accessorTypeFromStr(str);
        break; }
    }
    return true;
}

bool GLTFLoader4::handleAsset(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    switch(l->crumb().key[0]) {
    // copyright
    case 'c': {  g->copyright  = g->strings->writeStr(str, len); break; }
    // generator
    case 'g': {  g->generator  = g->strings->writeStr(str, len); break; }
    // version
    case 'v': {  g->version    = g->strings->writeStr(str, len); break; }
    // minVersion
    case 'm': {  g->minVersion = g->strings->writeStr(str, len); break; }
    }
    return true;
}

bool GLTFLoader4::handleAnimation(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    switch(*(uint16_t *)c.key) {
    // channels
    case 'c'|'h'<<8: {
        uint16_t aniIndex = l->crumb(-1).index;
        g->animations[aniIndex].channels = g->animationChannels + l->nextAnimationChannel;
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAnimationChannel;
        };
        c.handleEnd = [](GLTFLoader4 * l, Gobj * g, uint32_t count) {
            uint16_t aniIndex = l->crumb(-1).index;
            g->animations[aniIndex].nChannels = count;
            l->nextAnimationChannel += count;
        };
        break; }
    // samplers
    case 's'|'a'<<8: {
        uint16_t aniIndex = l->crumb(-1).index;
        g->animations[aniIndex].samplers = g->animationSamplers + l->nextAnimationSampler;
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAnimationSampler;
        };
        c.handleEnd = [](GLTFLoader4 * l, Gobj * g, uint32_t count) {
            uint16_t aniIndex = l->crumb(-1).index;
            g->animations[aniIndex].nSamplers = count;
            l->nextAnimationSampler += count;
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        g->animations[l->crumb(-1).index].name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader4::handleAnimationChannel(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    switch (c.key[0]) {
    // sampler
    case 's': {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-1).index;
        ac->sampler = g->animations[aniIndex].samplers + Number{str, len};
        break; }
    // target
    case 't': {
        // handle target object children
        c.handleChild = [](GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
            auto & c = l->crumb();
            uint16_t aniIndex = l->crumb(-4).index;
            Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-2).index;
            switch (c.key[0]) {
            // sampler
            case 'n': {
                ac->node = g->nodes + Number{str, len};
                break; }
            case 'p': {
                ac->path = l->animationTargetFromStr(str);
                break; }
            }
        };
        break; }
    }
    return true;
}

bool GLTFLoader4::handleAnimationSampler(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    uint16_t aniIndex = l->crumb(-3).index;
    Gobj::AnimationSampler * as = g->animations[aniIndex].samplers + l->crumb(-1).index;
    switch(*(uint32_t *)c.key) {
    // input
    case 'i'|'n'<<8|'p'<<16|'u'<<24: {
        as->input  = g->accessors + Number{str, len};
        break; }
    // interpolation
    case 'i'|'n'<<8|'t'<<16|'e'<<24: {
        as->interpolation = l->interpolationFromStr(str);
        break; }
    // output
    case 'o'|'u'<<8|'t'<<16|'p'<<24: {
        as->output = g->accessors + Number{str, len};
        break; }
    }
    return true;
}

bool GLTFLoader4::handleBuffer(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    uint16_t bufIndex = l->crumb(-1).index;
    Gobj::Buffer * buf = g->buffers + bufIndex;
    switch(c.key[0]) {
    // uri
    case 'u': {
        // copy data and set byteLength for non-GLB
        if (l->isGLB) {
            fprintf(stderr, "Unexpected buffer.uri in GLB.\n");
            return false;
        }
        size_t bytesWritten = l->handleData(l->nextRawDataPtr, str, len);
        if (bytesWritten == 0) {
            fprintf(stderr, "No bytes written in buffer %d.\n", bufIndex);
            return false;
        }
        buf->data = l->nextRawDataPtr;
        buf->byteLength = bytesWritten;
        l->nextRawDataPtr += bytesWritten;

        break; }
    // byteLength
    case 'b': {
        // copy data and set byteLength for GLB
        if (l->isGLB && bufIndex == 0) {
            Number n{str, len};
            buf->byteLength = n;
            if ((uint32_t)n != l->binDataSize()) {
                fprintf(stderr, "Unexpected buffer size.\n");
                return false;
            }
            memcpy(l->nextRawDataPtr, l->binData(), n);
            g->buffers[bufIndex].data = l->nextRawDataPtr;
            l->nextRawDataPtr += n;
        }

        break; }
    // name
    case 'n': {
        buf->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader4::handleBufferView(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    uint16_t bvIndex = l->crumb(-1).index;
    Gobj::BufferView * bv = g->bufferViews + bvIndex;
    auto & c = l->crumb();
    switch (c.key[0]) {
    case 'b': {
        switch (c.key[4]) {
        // buffer
        case 'e': {
            bv->buffer = g->buffers + Number{str, len};
            break; }
        // byteLength
        case 'L': {
            bv->byteLength = Number{str, len};
            break; }
        // byteOffset
        case 'O': {
            bv->byteOffset = Number{str, len};
            break; }
        // byteStride
        case 'S': {
            bv->byteStride = Number{str, len};
            break; }
        }
        break; }
    // target
    case 't': {
        bv->target = (Gobj::BufferView::Target)(int)Number{str, len};
        break; }
    // name
    case 'n': {
        bv->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader4::handleCamera(GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    switch (c.key[0]) {
    case 'b': {
        break; }
    }
    return true;
}
