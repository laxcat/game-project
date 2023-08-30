#include "GLTFLoader4.h"
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include "mem_utils.h"
#include "../common/file_utils.h"
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
    l->push(TYPE_UINT);
    // l->printBreadcrumbs();

    if (l->crumb(-2).matches(TYPE_ARR, "buffers") &&
        l->crumb().matches("byteLength")
    ) {
        l->counts.buffersLen += i;
    }

    l->pop();
    return true;
}

bool GLTFLoader4::Counter::String(char const * str, uint32_t length, bool copy) {
    l->push(TYPE_STR);
    // l->printBreadcrumbs();

    Crumb & thisCrumb = l->crumb();
    // name strings
    if (l->crumb(-2).matches(TYPE_ARR,
            "accessors|animations|buffers|bufferViews|cameras|images|materials|"
            "meshes|nodes|samplers|scenes|skins|textures") &&
        thisCrumb.matches("name")
    ) {
        l->counts.allStrLen += length + 1;
    }

    // asset strings
    else if (
        l->crumb(-1).matches(TYPE_OBJ, "asset") &&
        thisCrumb.matches("copyright|generator|version|minVersion")
    ) {
        l->counts.allStrLen += length + 1;
    }

    l->pop();
    return true;
}


bool GLTFLoader4::Counter::StartObject() {
    l->push(TYPE_OBJ);
    // l->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Counter::Key(char const * str, uint32_t length, bool copy) {
    snprintf(l->key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader4::Counter::EndObject(uint32_t memberCount) {
    l->pop();
    return true;
}

bool GLTFLoader4::Counter::StartArray() {
    l->push(TYPE_ARR);
    l->pushIndex();
    // l->printBreadcrumbs();
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
    l->pop();
    l->popIndex();
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //

GLTFLoader4::Scanner::Scanner(GLTFLoader4 * loader, Gobj * gobj) :
    l(loader),
    g(gobj)
{
    nextBufferPtr = g->buffer;

    #if DEBUG
    // write pretty JSON into string buffer
    uint32_t length;
    char const * str = l->prettyJSON(&length);
    g->jsonStr = g->strings->writeStr(str, length);
    #endif // DEBUG
}

bool GLTFLoader4::Scanner::Null() {
    l->push(TYPE_NULL);
    l->printBreadcrumbs();
    l->pop();
    return true;
}
bool GLTFLoader4::Scanner::Int(int i) {
    l->push(TYPE_INT);
    l->printBreadcrumbs();
    if      (l->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = l->crumb(-2).index;
        uint16_t minMaxIndex = l->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %d (int)", accIndex, l->crumb(-1).key, minMaxIndex, i);
        if      (l->crumb(-1).matches("min")) { g->accessors[accIndex].min[minMaxIndex] = (float)i; }
        else if (l->crumb(-1).matches("max")) { g->accessors[accIndex].max[minMaxIndex] = (float)i; }
    }
    l->pop();
    return true;
}
bool GLTFLoader4::Scanner::Int64(int64_t i) {
    l->push(TYPE_INT);
    l->printBreadcrumbs();
    l->pop();
    return true;
}
bool GLTFLoader4::Scanner::Uint64(uint64_t i) {
    l->push(TYPE_UINT);
    l->printBreadcrumbs();
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::RawNumber(char const * str, uint32_t length, bool copy) { return true; }

bool GLTFLoader4::Scanner::Bool(bool b) {
    l->push(TYPE_BOOL);
    l->printBreadcrumbs();
    if (l->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = l->crumb(-1).index;
        if     (l->crumb().matches("normalized"))     { g->accessors[index].normalized = b; }
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::Uint(unsigned i) {
    l->push(TYPE_UINT);
    l->printBreadcrumbs();
    if (l->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = l->crumb(-1).index;
        if      (l->crumb().matches("bufferView"))     { g->accessors[index].bufferView = g->bufferViews + i; }
        else if (l->crumb().matches("byteOffset"))     { g->accessors[index].byteOffset = i; }
        else if (l->crumb().matches("componentType"))  { g->accessors[index].componentType = (Gobj::Accessor::ComponentType)i; }
        else if (l->crumb().matches("normalized"))     { g->accessors[index].normalized = i; }
        else if (l->crumb().matches("count"))          { g->accessors[index].count = i; }

    }
    else if (l->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = l->crumb(-2).index;
        uint16_t minMaxIndex = l->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %d (uint)", accIndex, l->crumb(-1).key, minMaxIndex, i);
        if      (l->crumb(-1).matches("min")) { g->accessors[accIndex].min[minMaxIndex] = (float)i; }
        else if (l->crumb(-1).matches("max")) { g->accessors[accIndex].max[minMaxIndex] = (float)i; }
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "animations") &&
        l->crumb(-2).matches(TYPE_ARR, "channels") &&
        l->crumb().matches("sampler"))
    {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-1).index;
        ac->sampler = g->animations[aniIndex].samplers + i;
    }
    else if (
        l->crumb(-5).matches(TYPE_ARR, "animations") &&
        l->crumb(-3).matches(TYPE_ARR, "channels") &&
        l->crumb(-1).matches(TYPE_OBJ, "target") &&
        l->crumb().matches("node"))
    {
        uint16_t aniIndex = l->crumb(-4).index;
        Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-2).index;
        ac->node = g->nodes + i;
    }
    else if (
        l->crumb(-4).matches(TYPE_ARR, "animations") &&
        l->crumb(-2).matches(TYPE_ARR, "samplers"))
    {
        uint16_t aniIndex = l->crumb(-3).index;
        Gobj::AnimationSampler * as = g->animations[aniIndex].samplers + l->crumb(-1).index;
        if      (l->crumb().matches("input"))  { as->input  = g->accessors + i; }
        else if (l->crumb().matches("output")) { as->output = g->accessors + i; }
    }
    else if (
        l->crumb(-2).matches(TYPE_ARR, "buffers") &&
        l->crumb().matches("byteLength"))
    {
        uint16_t bufIndex = l->crumb(-1).index;
        g->buffers[bufIndex].byteLength = i;
        g->buffers[bufIndex].data = nextBufferPtr;
        nextBufferPtr += i;
    }
    else if (l->crumb(-2).matches(TYPE_ARR, "bufferViews")) {
        uint16_t bvIndex = l->crumb(-1).index;
        Gobj::BufferView * bv = g->bufferViews + bvIndex;
        if      (l->crumb().matches("buffer"))     { bv->buffer = g->buffers + i; }
        else if (l->crumb().matches("byteLength")) { bv->byteLength = i; }
        else if (l->crumb().matches("byteOffset")) { bv->byteOffset = i; }
        else if (l->crumb().matches("byteStride")) { bv->byteStride = i; }
        else if (l->crumb().matches("target"))     { bv->target = (Gobj::BufferView::Target)i; }
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::Double(double d) {
    l->push(TYPE_FLOAT);
    l->printBreadcrumbs();
    if      (l->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = l->crumb(-2).index;
        uint16_t minMaxIndex = l->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %f (float)", accIndex, l->crumb(-1).key, minMaxIndex, d);
        if      (l->crumb(-1).matches("min")) { g->accessors[accIndex].min[minMaxIndex] = (float)d; }
        else if (l->crumb(-1).matches("max")) { g->accessors[accIndex].max[minMaxIndex] = (float)d; }
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::String(char const * str, uint32_t length, bool copy) {
    l->push(TYPE_STR);
    l->printBreadcrumbs();
    // handle name strings
    if (l->crumb().matches("name")) {
        char const * key = l->crumb(-2).key;
        uint16_t index = l->crumb(-1).index;
        if      (strEqu(key, "accessors"))  { g->accessors[index].name   = g->strings->writeStr(str, length); }
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
        if      (l->crumb().matches("copyright"))  { g->copyright   = g->strings->writeStr(str, length); }
        else if (l->crumb().matches("generator"))  { g->generator   = g->strings->writeStr(str, length); }
        else if (l->crumb().matches("version"))    { g->version     = g->strings->writeStr(str, length); }
        else if (l->crumb().matches("minVersion")) { g->minVersion  = g->strings->writeStr(str, length); }
    }
    else if (l->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = l->crumb(-1).index;
        if     (l->crumb().matches("type")) { g->accessors[index].type = l->accessorTypeFromStr(str); }

    }
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
            if (l->isGLB) {
                fprintf(stderr, "Unexpected buffer.uri in GLB.");
                return false;
            }
            size_t bytesWritten = l->handleData(nextBufferPtr, str, length);
            if (bytesWritten == 0) {
                return false;
            }
            Gobj::Buffer * buf = g->buffers + l->crumb(-1).index;
            buf->data = nextBufferPtr;
            buf->byteLength = bytesWritten;
            nextBufferPtr += bytesWritten;
    }
    l->pop();
    return true;
}

bool GLTFLoader4::Scanner::StartObject() {
    l->push(TYPE_OBJ);
    l->printBreadcrumbs();
    if (l->crumb(-1).matches(TYPE_ARR, "animations")) {
        uint16_t index = l->crumb().index;
        g->animations[index].channels = g->animationChannels + nextAnimationChannel;
        g->animations[index].samplers = g->animationSamplers + nextAnimationSampler;
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
    l->pushIndex();
    l->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Scanner::EndArray(uint32_t elementCount) {
    if (l->crumb(-2).matches(TYPE_ARR, "animations")) {
        uint16_t aniIndex = l->crumb(-1).index;
        if (l->crumb().matches("channels")) {
            g->animations[aniIndex].nChannels = elementCount;
            nextAnimationChannel += elementCount;
        }
        else if (l->crumb().matches("samplers")) {
            g->animations[aniIndex].nSamplers = elementCount;
            nextAnimationSampler += elementCount;
        }
    }
    l->pop();
    l->popIndex();
    return true;
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
    return
        (objType == TYPE_OBJ)   ? "OBJ" :
        (objType == TYPE_ARR)   ? "ARR" :
        (objType == TYPE_STR)   ? "STR" :
        (objType == TYPE_NULL)  ? "NULL" :
        (objType == TYPE_BOOL)  ? "BOOL" :
        (objType == TYPE_UINT)  ? "UINT" :
        (objType == TYPE_INT)   ? "INT" :
        (objType == TYPE_FLOAT) ? "FLOAT" :
        "UNKNOWN";
}

bool GLTFLoader4::Crumb::hasKey() const { return (key[0] != '\0'); }

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
            fprintf(stderr, "glTF JSON chunk magic string not found.");
            gltfData = nullptr;
            return;
        }
        if (!strEqu((char const *)(binChunkStart() + 4), "BIN\0", 4)) {
            fprintf(stderr, "BIN chunk magic string not found.");
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
    reader.Parse(ss, counter);
}

bool GLTFLoader4::load(Gobj * gobj) {
    assert(gltfData && "GLTF data invalid.");
    Scanner scanner{this, gobj};
    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr());
    bool success = reader.Parse(ss, scanner);
    if (isGLB && success) {
        memcpy(gobj->buffer, binData(), binDataSize());
    }
    return success;
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
        print("%s(%s,%d) > ", crumbs[i].objTypeStr(), crumbs[i].key, crumbs[i].index);
    }
    printl();
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

void GLTFLoader4::push(ObjType objType) {
    assert(depth < MaxDepth && "Can't push crumb.");

    // if parent is an array, mark the index in newly pushed child and increntment next index.
    if (depth && crumbs[depth-1].objType == TYPE_ARR) {
        crumbs[depth].index = arrIndices[arrDepth-1];
        ++arrIndices[arrDepth-1];
    }

    crumbs[depth].objType = objType;
    // copy and consume key
    crumbs[depth].setKey(key);
    key[0] = '\0';
    ++depth;
}

void GLTFLoader4::pop() {
    --depth;
}

void GLTFLoader4::pushIndex() {
    arrIndices[arrDepth] = 0;
    ++arrDepth;
}

void GLTFLoader4::popIndex() {
    --arrDepth;
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
