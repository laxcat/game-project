#include "GLTFLoader.h"
#include <glm/gtc/type_ptr.hpp>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include "mem_utils.h"
#include "../common/file_utils.h"
#include "../common/Number.h"
#include "../common/modp_b64.h"
#include "../common/string_utils.h"
#include "../MrManager.h"

#define PRINT_BREADCRUMBS 0

// -------------------------------------------------------------------------- //
// COUNTER
// -------------------------------------------------------------------------- //

bool GLTFLoader::Counter::Null() { return true; }
bool GLTFLoader::Counter::Bool(bool b) { return true; }
bool GLTFLoader::Counter::Int(int i) { return true; }
bool GLTFLoader::Counter::Int64(int64_t i) { return true; }
bool GLTFLoader::Counter::Uint64(uint64_t i) { return true; }
bool GLTFLoader::Counter::Double(double d) { return true; }
bool GLTFLoader::Counter::RawNumber(char const * str, uint32_t length, bool copy) { return true; }

bool GLTFLoader::Counter::Uint(unsigned i) {
    if (l->crumb(-1).matches(TYPE_ARR, "buffers") &&
        strEqu(l->_key, "byteLength")
    ) {
        l->_counts.rawDataLen += i;
    }
    return true;
}

bool GLTFLoader::Counter::String(char const * str, uint32_t length, bool copy) {
    // name strings (except scene, which requires name, so has char array)
    if (strEqu(l->_key, "name") &&
        l->crumb(-1).matches(TYPE_ARR,
            "accessors|animations|buffers|bufferViews|cameras|images|materials|"
            "meshes|nodes|samplers|skins|textures")

    ) {
        l->_counts.allStrLen += length + 1;
    }

    // asset strings
    else if (
        l->crumb().matches(TYPE_OBJ, "asset") &&
        strWithin(l->_key, "copyright|generator|version|minVersion", '|')
    ) {
        l->_counts.allStrLen += length + 1;
    }
    return true;
}


bool GLTFLoader::Counter::StartObject() {
    l->push(TYPE_OBJ);
    return true;
}

bool GLTFLoader::Counter::Key(char const * str, uint32_t length, bool copy) {
    snprintf(l->_key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader::Counter::EndObject(uint32_t memberCount) {
    if (
        (l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches("primitives") &&
        l->crumb().matches("attributes"))
        ||
        (l->crumb(-5).matches(TYPE_ARR, "meshes") &&
        l->crumb(-3).matches("primitives") &&
        l->crumb(-1).matches("targets"))
    ) {
        l->_counts.meshAttributes += memberCount;
    }
    l->pop();
    return true;
}

bool GLTFLoader::Counter::StartArray() {
    l->push(TYPE_ARR);
    return true;
}

bool GLTFLoader::Counter::EndArray(uint32_t elementCount) {
    Crumb & crumb = l->crumb();
    // count the main sub-objects
    if (l->_depth == 2) {
        if      (crumb.matches("accessors"  )) { l->_counts.accessors   = elementCount; }
        else if (crumb.matches("animations" )) { l->_counts.animations  = elementCount; }
        else if (crumb.matches("buffers"    )) { l->_counts.buffers     = elementCount; }
        else if (crumb.matches("bufferViews")) { l->_counts.bufferViews = elementCount; }
        else if (crumb.matches("cameras"    )) { l->_counts.cameras     = elementCount; }
        else if (crumb.matches("images"     )) { l->_counts.images      = elementCount; }
        else if (crumb.matches("materials"  )) { l->_counts.materials   = elementCount; }
        else if (crumb.matches("meshes"     )) { l->_counts.meshes      = elementCount; }
        else if (crumb.matches("nodes"      )) { l->_counts.nodes       = elementCount; }
        else if (crumb.matches("samplers"   )) { l->_counts.samplers    = elementCount; }
        else if (crumb.matches("scenes"     )) { l->_counts.scenes      = elementCount; }
        else if (crumb.matches("skins"      )) { l->_counts.skins       = elementCount; }
        else if (crumb.matches("textures"   )) { l->_counts.textures    = elementCount; }
    }
    // node children and scene children
    else if (
        l->_depth == 4 &&
        ((crumb.matches("nodes") && l->crumb(-2).matches("scenes"))
        ||
        (crumb.matches("children") && l->crumb(-2).matches("nodes")))
    ) {
        l->_counts.nodeChildren += elementCount;
    }
    // animation channels and samplers
    else if (l->_depth == 4 && l->crumb(-2).matches("animations")) {
        if      (crumb.matches("channels")) { l->_counts.animationChannels += elementCount; }
        else if (crumb.matches("samplers")) { l->_counts.animationSamplers += elementCount; }
    }
    // mesh primitives and weights
    else if (l->_depth == 4 && l->crumb(-2).matches("meshes")) {
        if      (crumb.matches("primitives")) { l->_counts.meshPrimitives += elementCount; }
        else if (crumb.matches("weights")) { l->_counts.meshWeights += elementCount; }
    }
    else if (l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches(TYPE_ARR, "primitives") &&
        l->crumb().matches(TYPE_ARR, "targets"))
    {
        l->_counts.meshTargets += elementCount;
    }
    else if (l->_depth == 4 && l->crumb(-2).matches("nodes") && crumb.matches("weights")) {
        l->_counts.nodeWeights += elementCount;
    }
    l->pop();
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //

bool GLTFLoader::Scanner::Null() { return false; }
bool GLTFLoader::Scanner::Int(int i) { return false; }
bool GLTFLoader::Scanner::Int64(int64_t i) { return false; }
bool GLTFLoader::Scanner::Uint64(uint64_t i) { return false; }
bool GLTFLoader::Scanner::Uint(unsigned i) { return false; }
bool GLTFLoader::Scanner::Double(double d) { return false; }

bool GLTFLoader::Scanner::Bool(bool b) {
    return Value(TYPE_NUM, b?"1":"0", 1);
}
bool GLTFLoader::Scanner::RawNumber(char const * str, uint32_t length, bool copy) {
    return Value(TYPE_NUM, str, length);
}
bool GLTFLoader::Scanner::String(char const * str, uint32_t length, bool copy) {
    return Value(TYPE_STR, str, length);
}

bool GLTFLoader::Scanner::Value(ObjType type, char const * str, uint32_t length) {
    l->push(type);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        bool success = l->crumb(-1).handleChild(l, g, str, length);
        if (!success) return false;
    }

    #if DEBUG && PRINT_BREADCRUMBS
    l->printBreadcrumbs();
    if (type == TYPE_STR) {
        printl("\"%.*s\"", length, str);
    }
    else {
        printl("%.*s", length, str);
    }
    #endif // DEBUG

    l->pop();
    return true;
}

bool GLTFLoader::Scanner::StartObject() {
    l->push(TYPE_OBJ);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        bool success = l->crumb(-1).handleChild(l, g, nullptr, 0);
        if (!success) return false;
    }
    // set new handle child
    if (l->_depth == 1) {
        l->crumb().handleChild = handleRoot;
    }

    #if DEBUG && PRINT_BREADCRUMBS
    l->printBreadcrumbs();
    printl();
    #endif // DEBUG
    return true;
}

bool GLTFLoader::Scanner::Key(char const * str, uint32_t length, bool copy) {
    snprintf(l->_key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader::Scanner::EndObject(uint32_t memberCount) {
    if (l->crumb().handleEnd) {
        bool success = l->crumb().handleEnd(l, g, memberCount);
        if (!success) return false;
    }
    l->pop();
    return true;
}

bool GLTFLoader::Scanner::StartArray() {
    l->push(TYPE_ARR);
    if (l->crumb(-1).handleChild) {
        bool success = l->crumb(-1).handleChild(l, g, nullptr, 0);
        if (!success) return false;
    }
    #if DEBUG && PRINT_BREADCRUMBS
    l->printBreadcrumbs();
    printl();
    #endif // DEBUG
    return true;
}

bool GLTFLoader::Scanner::EndArray(uint32_t elementCount) {
    if (l->crumb().handleEnd) {
        bool success = l->crumb().handleEnd(l, g, elementCount);
        if (!success) return false;
    }
    l->pop();
    return true;
}

// -------------------------------------------------------------------------- //
// CRUMB
// -------------------------------------------------------------------------- //

GLTFLoader::Crumb::Crumb(ObjType objType, char const * key) {
    this->objType = objType;
    setKey(key);
}

GLTFLoader::Crumb::Crumb() {}

void GLTFLoader::Crumb::setKey(char const * key) {
    if (key) {
        fixstrcpy<MaxKeyLen>(this->key, key);
    }
    else {
        this->key[0] = '\0';
    }
}

bool GLTFLoader::Crumb::matches(char const * key) const {
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

bool GLTFLoader::Crumb::matches(ObjType objType, char const * key) const {
    if (objType != this->objType) return false;
    return matches(key);
}

char const * GLTFLoader::Crumb::objTypeStr() const {
    return GLTFLoader::objTypeStr(objType);
}

bool GLTFLoader::Crumb::hasKey() const { return (key[0] != '\0'); }

bool GLTFLoader::Crumb::isValid() const { return (objType != TYPE_UNKNOWN); }

// -------------------------------------------------------------------------- //
// LOADER
// -------------------------------------------------------------------------- //

GLTFLoader::GLTFLoader(byte_t const * gltfData, char const * loadingDir) :
    _gltfData(gltfData),
    _loadingDir(loadingDir)
{
    // ensure GLTF expected alignment
    assert(alignPtr((void *)_gltfData, 4) == _gltfData && "GLTF data must be 4-byte aligned.");

    // set loader pointer to json handlers
    _scanner.l = this;
    _counter.l = this;

    // binary
    if (strEqu((char const *)_gltfData, "glTF", 4)) {
        _isGLB = true;

        if (!strEqu((char const *)(_gltfData + 16), "JSON", 4)) {
            fprintf(stderr, "glTF JSON chunk magic string not found.\n");
            _gltfData = nullptr;
            return;
        }
        if (!strEqu((char const *)(binChunkStart() + 4), "BIN\0", 4)) {
            fprintf(stderr, "BIN chunk magic string not found.\n");
            _gltfData = nullptr;
            return;
        }
    }
    // not binary
    else {
    }
}

Gobj::Counts GLTFLoader::counts() const { return _counts; }
bool GLTFLoader::isGLB() const { return _isGLB; }

GLTFLoader::Crumb & GLTFLoader::crumb(int offset) {
    // out of bounds, return invalid crumb
    if (_depth <= -offset || offset > 0) {
        return _invalidCrumb;
    }
    return _crumbs[_depth-1+offset];
}

size_t GLTFLoader::calculateSize() {
    assert(_gltfData && "GLTF data invalid.");

    #if DEBUG
    // add length of pretty JSON string to total string length
    prettyJSON(&_counts.allStrLen);
    #endif // DEBUG

    auto ss = rapidjson::StringStream(jsonStr());
    _reader.Parse<rapidjson::kParseStopWhenDoneFlag>(ss, _counter);
    return _counts.totalSize();
}

bool GLTFLoader::load(Gobj * gobj) {
    // using namespace rapidjson;
    assert(_gltfData && "GLTF data invalid.");

    _key[0] = '\0';
    _scanner.g = gobj;
    _nextRawDataPtr = gobj->rawData;

    #if DEBUG
    // write pretty JSON into string buffer
    uint32_t length;
    char const * str = prettyJSON(&length);
    gobj->jsonStr = gobj->strings->writeStr(str, length);
    #endif // DEBUG

    auto ss = rapidjson::StringStream(jsonStr());
    constexpr static unsigned parseFlags =
        rapidjson::kParseStopWhenDoneFlag |
        rapidjson::kParseNumbersAsStringsFlag;
    rapidjson::ParseResult result = _reader.Parse<parseFlags>(ss, _scanner);
    if (result.IsError()) {
        fprintf(stderr, "JSON parse error: %s (%zu)\n",
            rapidjson::GetParseError_En(result.Code()), result.Offset());
    }
    postLoad(gobj);
    return result;
}

void GLTFLoader::postLoad(Gobj * g) {
    // set byte stride on all mesh primitive attributes
    for (uint16_t meshIndex = 0; meshIndex < g->counts.meshes; ++meshIndex) {
        auto & mesh = g->meshes[meshIndex];
        for (uint16_t primIndex = 0; primIndex < mesh.nPrimitives; ++primIndex) {
            Gobj::MeshPrimitive & prim = mesh.primitives[primIndex];
            for (size_t attrIndex = 0; attrIndex < prim.nAttributes; ++attrIndex) {
                Gobj::MeshAttribute & attr = prim.attributes[attrIndex];
                Gobj::Accessor & acc = *attr.accessor;
                Gobj::BufferView & bv = *acc.bufferView;
                if (bv.byteStride == 0) {
                    bv.byteStride = acc.byteSize();
                }
            }
        }
    }

    // set scene if scenes are present but no default set
    if (g->scene == nullptr && g->counts.scenes > 0 && g->scenes) {
        g->scene = g->scenes;
    }

    // fill in generic name for scenes without names
    for (uint16_t scnIndex = 0; scnIndex < g->counts.scenes && scnIndex < 100; ++scnIndex) {
        Gobj::Scene & s = g->scenes[scnIndex];
        if (s.name[0] == '\0') {
            snprintf(s.name, Gobj::Scene::NameSize, "Scene%02u", scnIndex);
        }
    }

    // set matrix <-> TRS on all nodes with meshes
    for (uint16_t nodeIndex = 0; nodeIndex < g->scene->nNodes; ++nodeIndex) {
        g->scene->nodes[nodeIndex]->syncMatrixTRS(true);
    }
}

#if DEBUG
char const * GLTFLoader::prettyJSON(uint32_t * prettyJSONSize) {
    assert(_gltfData && "GLTF data invalid.");

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    auto ss = rapidjson::StringStream(jsonStr());
    _reader.Parse(ss, writer);

    if (prettyJSONSize) {
        *prettyJSONSize = sb.GetSize() + 1;
    }
    return mm.frameFormatStr("%s", sb.GetString());
}
#endif // DEBUG

#if DEBUG
void GLTFLoader::printBreadcrumbs() const {
    if (_depth < 1) return;
    for (int i = 0; i < _depth; ++i) {
        if (_crumbs[i].index == -1) {
            if (_crumbs[i].key[0]) {
                print("%s:", _crumbs[i].key);
            }
            print("%s > ", _crumbs[i].objTypeStr());
        }
        else {
            print("%d:%s > ", _crumbs[i].index, _crumbs[i].objTypeStr());
        }
    }
}
#endif // DEBUG

uint32_t GLTFLoader::jsonStrSize() const {
    assert(_isGLB && "jsonStrSize() not available when _isGLB==false.");
    return *(uint32_t *)(_gltfData + 12);
}

char const * GLTFLoader::jsonStr() const {
    // returns _gltfData if GLTF,
    // returns _gltfData+20 if GLB
    return (char const *)(_gltfData + (_isGLB * 20));
}

byte_t const * GLTFLoader::binChunkStart() const {
    assert(_isGLB && "No binary chuck to access.");
    // aligned because GLTF spec will add space characters at end of json
    // string in order to align to 4-byte boundary
    auto jsonPtr = jsonStr() + jsonStrSize();
    auto alignedPtr = (byte_t const *)alignPtr((void *)jsonPtr, 4);
    return alignedPtr;
}

uint32_t GLTFLoader::binDataSize() const {
    return *(uint32_t *)binChunkStart();
}

byte_t const * GLTFLoader::binData() const {
    return binChunkStart() + 8;
}

bool GLTFLoader::validData() const {
    return _gltfData;
}

char const * GLTFLoader::objTypeStr(ObjType objType) {
    return
        (objType == TYPE_OBJ)   ? "OBJ" :
        (objType == TYPE_ARR)   ? "ARR" :
        (objType == TYPE_STR)   ? "STR" :
        (objType == TYPE_NUM)   ? "NUM" :
        "UNKNOWN";
}

void GLTFLoader::push(ObjType objType) {
    assert(_depth < MaxDepth && "Can't push crumb.");

    // push new crumb
    ++_depth;
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
    newCrumb.setKey(_key);
    _key[0] = '\0';
    // reset everthing
    newCrumb.childCount = 0;
    newCrumb.handleChild = nullptr;
    newCrumb.handleEnd = nullptr;
}

void GLTFLoader::pop() {
    --_depth;
}

size_t GLTFLoader::handleDataString(byte_t * dst, char const * str, size_t strLength) {
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
    char * fullPath = mm.frameFormatStr("%s%s", _loadingDir, str);
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

// -------------------------------------------------------------------------- //
// HANDLERS
// -------------------------------------------------------------------------- //

bool GLTFLoader::handleRoot(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    switch(*(uint16_t *)c.key) {
    // accessors
    case 'a'|'c'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleAccessor;
            return true;
        };
        break; }
    // asset
    case 'a'|'s'<<8: {
        c.handleChild = handleAsset;
        break; }
    // animations
    case 'a'|'n'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleAnimation;
            return true;
        };
        break; }
    // buffers|bufferViews
    case 'b'|'u'<<8: {
        switch(c.key[6]) {
        // buffers
        case 's': {
            c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
                l->crumb().handleChild = handleBuffer;
                return true;
            };
            break; }
        // bufferViews
        case 'V': {
            c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
                l->crumb().handleChild = handleBufferView;
                return true;
            };
            break; }
        }
        break; }
    // cameras
    case 'c'|'a'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleCamera;
            return true;
        };
        break; }
    // images
    case 'i'|'m'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleImage;
            return true;
        };
        break; }
    // materials
    case 'm'|'a'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleMaterial;
            return true;
        };
        break; }
    // meshes
    case 'm'|'e'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleMesh;
            return true;
        };
        break; }
    // nodes
    case 'n'|'o'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleNode;
            return true;
        };
        break; }
    // samplers
    case 's'|'a'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleSampler;
            return true;
        };
        break; }
    // scenes
    case 's'|'c'<<8: {
        switch (c.key[5]) {
        case '\0': {
            g->scene = g->scenes + Number{str, len};
            break; }
        case 's': {
            c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
                l->crumb().handleChild = handleScene;
                return true;
            };
            break; }
        }
        break; }
    // skins
    case 's'|'k'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleSkin;
            return true;
        };
        break; }
    // textures
    case 't'|'e'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj *, char const *, uint32_t) {
            l->crumb().handleChild = handleTexture;
            return true;
        };
        break; }
    }
    return true;
}

bool GLTFLoader::handleAccessor(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
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
        c.handleChild = [](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            uint16_t accIndex = l->crumb(-2).index;
            uint16_t maxIndex = l->crumb().index;
            g->accessors[accIndex].max[maxIndex] = Number{str, len};
            return true;
        };
        break; }
    // min
    case 'm'|'i'<<8: {
        c.handleChild = [](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            uint16_t accIndex = l->crumb(-2).index;
            uint16_t minIndex = l->crumb().index;
            g->accessors[accIndex].min[minIndex] = Number{str, len};
            return true;
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
        a->type = Gobj::accessorTypeFromStr(str);
        break; }
    }
    return true;
}

bool GLTFLoader::handleAsset(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
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

bool GLTFLoader::handleAnimation(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    uint16_t aniIndex = l->crumb(-1).index;
    switch(*(uint16_t *)c.key) {
    // channels
    case 'c'|'h'<<8: {
        g->animations[aniIndex].channels = g->animationChannels + l->_nextAnimationChannel;
        c.handleChild = [](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAnimationChannel;
            return true;
        };
        c.handleEnd = [aniIndex](GLTFLoader * l, Gobj * g, uint32_t count) {
            g->animations[aniIndex].nChannels = count;
            l->_nextAnimationChannel += count;
            return true;
        };
        break; }
    // samplers
    case 's'|'a'<<8: {
        g->animations[aniIndex].samplers = g->animationSamplers + l->_nextAnimationSampler;
        c.handleChild = [](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleAnimationSampler;
            return true;
        };
        c.handleEnd = [aniIndex](GLTFLoader * l, Gobj * g, uint32_t count) {
            g->animations[aniIndex].nSamplers = count;
            l->_nextAnimationSampler += count;
            return true;
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        g->animations[l->crumb(-1).index].name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader::handleAnimationChannel(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
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
        c.handleChild = [](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            auto & c = l->crumb();
            uint16_t aniIndex = l->crumb(-4).index;
            Gobj::AnimationChannel * ac = g->animations[aniIndex].channels + l->crumb(-2).index;
            switch (c.key[0]) {
            // sampler
            case 'n': {
                ac->node = g->nodes + Number{str, len};
                break; }
            // path
            case 'p': {
                ac->path = Gobj::animationTargetFromStr(str);
                break; }
            }
            return true;
        };
        break; }
    }
    return true;
}

bool GLTFLoader::handleAnimationSampler(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
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
        as->interpolation = Gobj::interpolationFromStr(str);
        break; }
    // output
    case 'o'|'u'<<8|'t'<<16|'p'<<24: {
        as->output = g->accessors + Number{str, len};
        break; }
    }
    return true;
}

bool GLTFLoader::handleBuffer(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    uint16_t bufIndex = l->crumb(-1).index;
    Gobj::Buffer * buf = g->buffers + bufIndex;
    switch(c.key[0]) {
    // uri
    case 'u': {
        // copy data and set byteLength for non-GLB
        if (l->_isGLB) {
            fprintf(stderr, "Unexpected buffer.uri in GLB.\n");
            return false;
        }
        size_t bytesWritten = l->handleDataString(l->_nextRawDataPtr, str, len);
        if (bytesWritten == 0) {
            fprintf(stderr, "No bytes written in buffer %d.\n", bufIndex);
            return false;
        }
        buf->data = l->_nextRawDataPtr;
        buf->byteLength = bytesWritten;
        l->_nextRawDataPtr += bytesWritten;

        break; }
    // byteLength
    case 'b': {
        // copy data and set byteLength for GLB
        if (l->_isGLB && bufIndex == 0) {
            Number n{str, len};
            buf->byteLength = n;
            if ((uint32_t)n != l->binDataSize()) {
                fprintf(stderr, "Unexpected buffer size.\n");
                return false;
            }
            memcpy(l->_nextRawDataPtr, l->binData(), n);
            g->buffers[bufIndex].data = l->_nextRawDataPtr;
            l->_nextRawDataPtr += n;
        }

        break; }
    // name
    case 'n': {
        buf->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader::handleBufferView(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
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

bool GLTFLoader::handleCamera(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Camera * cam = g->cameras + l->crumb(-1).index;
    switch (c.key[0]) {
    // orthographic
    case 'o': {
        cam->orthographic = (Gobj::CameraOrthographic *)&cam->_data;
        c.handleChild = [cam](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            Number n{str, len};
            switch (*(uint16_t *)l->crumb().key) {
            // xmag
            case 'x'|'m'<<8: { cam->_data[0] = n; break; }
            // ymag
            case 'y'|'m'<<8: { cam->_data[1] = n; break; }
            // zfar
            case 'z'|'f'<<8: { cam->_data[2] = n; break; }
            // znear
            case 'z'|'n'<<8: { cam->_data[3] = n; break; }
            }
            return true;
        };
        break; }
    // perspective
    case 'p': {
        cam->perspective = (Gobj::CameraPerspective *)&cam->_data;
        c.handleChild = [cam](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            Number n{str, len};
            switch (*(uint16_t *)l->crumb().key) {
            // aspectRatio
            case 'a'|'s'<<8: { cam->_data[0] = n; break; }
            // yfov
            case 'y'|'f'<<8: { cam->_data[1] = n; break; }
            // zfar
            case 'z'|'f'<<8: { cam->_data[2] = n; break; }
            // znear
            case 'z'|'n'<<8: { cam->_data[3] = n; break; }
            }
            return true;
        };
        break; }
    case 't': {
        cam->type = Gobj::cameraTypeFromStr(str);
        break; }
    case 'n': {
        cam->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader::handleImage(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Image * img = g->images + l->crumb(-1).index;
    switch (c.key[0]) {
    // uri
    case 'u': {
        fprintf(stderr,
            "WARNING, external images are not loaded to main memory. "
            "TODO: load directly to GPU or setup deferred loading.\n"
        );
        break; }
    // mimeType
    case 'm': {
        img->mimeType = Gobj::imageMIMETypeFromStr(str);
        break; }
    // bufferView
    case 'b': {
        img->bufferView = g->bufferViews + Number{str, len};
        break; }
    // name
    case 'n': {
        img->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader::handleMaterial(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    Gobj::Material * mat = g->materials + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (c.key[0]) {
    // alphaMode || alphaCutoff
    case 'a': {
        switch (c.key[5]) {
        case 'M': { mat->alphaMode = Gobj::alphaModeFromStr(str); break; }
        case 'C': { mat->alphaCutoff = Number{str, len}; break; }
        }
        break; }
    // emissiveTexture || emissiveFactor
    case 'e': {
        switch (c.key[8]) {
        // emissiveTexture
        case 'T': {
            c.handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                Number n{str, len};
                switch (l->crumb().key[0]) {
                // index
                case 'i': { mat->emissiveTexture = g->textures + n; break; }
                // texCoord
                case 't': { mat->emissiveTexCoord = (Gobj::Attr)(int)n; break; }
                }
                return true;
            };
            break; }
        // emissiveFactor
        case 'F': {
            c.handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                mat->emissiveFactor[l->crumb().index] = Number{str, len};
                return true;
            };
            break; }
        }
        break; }
    // doubleSided
    case 'd': {
        mat->doubleSided = Number{str, len};
        break; }
    // normalTexture || name
    case 'n': {
        switch (c.key[1]) {
        // normalTexture
        case 'o': {
            c.handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                Number n{str, len};
                switch (l->crumb().key[0]) {
                // index
                case 'i': { mat->normalTexture = g->textures + n; break; }
                // scale
                case 's': { mat->normalScale = n; break; }
                // texCoord
                case 't': { mat->normalTexCoord = (Gobj::Attr)(int)n; break; }
                }
                return true;
            };
            break; }
        // name
        case 'a': { mat->name = g->strings->writeStr(str, len); break; }
        }
        break; }
    // occlusionTexture
    case 'o': {
        c.handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            Number n{str, len};
            switch (l->crumb().key[0]) {
            // index
            case 'i': { mat->occlusionTexture = g->textures + n; break; }
            // strength
            case 's': { mat->occlusionStrength = n; break; }
            // texCoord
            case 't': { mat->occlusionTexCoord = (Gobj::Attr)(int)n; break; }
            }
            return true;
        };
        break; }
    // pbrMetallicRoughness
    case 'p': {
        c.handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            switch (l->crumb().key[0]) {
            // baseColorFactor || baseColorTexture
            case 'b': {
                switch (l->crumb().key[9]) {
                // baseColorFactor
                case 'F': {
                    l->crumb().handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                        mat->baseColorFactor[l->crumb().index] = Number{str, len};
                        return true;
                    };
                    break; }
                // baseColorTexture
                case 'T': {
                    l->crumb().handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                        Number n{str, len};
                        switch (l->crumb().key[0]) {
                        // index
                        case 'i': { mat->baseColorTexture = g->textures + n; break; }
                        // texCoord
                        case 't': { mat->baseColorTexCoord = (Gobj::Attr)(int)n; break; }
                        }
                        return true;
                    };
                    break; }
                }
                break; }
            // metallicFactor || metallicRoughnessTexture
            case 'm': {
                switch (l->crumb().key[8]) {
                // metallicFactor
                case 'F': { mat->metallicFactor = Number{str, len}; break; }
                // metallicRoughnessTexture
                case 'R': {
                    l->crumb().handleChild = [mat](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                        Number n{str, len};
                        switch (l->crumb().key[0]) {
                        // index
                        case 'i': { mat->metallicRoughnessTexture = g->textures + n; break; }
                        // texCoord
                        case 't': { mat->metallicRoughnessTexCoord = (Gobj::Attr)(int)n; break; }
                        }
                        return true;
                    };
                    break; }
                }
                break; }
            // roughnessFactor
            case 'r': { mat->roughnessFactor = Number{str, len}; break; }
            }
            return true;
        };
        break; }
    }
    return true;
}

bool GLTFLoader::handleMesh(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Mesh * mesh = g->meshes + l->crumb(-1).index;
    switch (c.key[0]) {
    // primitives
    case 'p': {
        // primitives array
        mesh->primitives = g->meshPrimitives + l->_nextMeshPrimitive;
        // handle each primitive
        c.handleChild = [mesh](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            l->crumb().handleChild = handleMeshPrimitive;
            return true;
        };
        // on end of primitives array
        c.handleEnd = [mesh](GLTFLoader * l, Gobj * g, uint32_t count) {
            mesh->nPrimitives = count;
            l->_nextMeshPrimitive += count;
            return true;
        };
        break; }
    // weights
    case 'w': {
        mesh->weights = g->meshWeights + l->_nextMeshWeight;
        c.handleChild = [mesh](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            mesh->weights[mesh->nWeights] = Number{str, len};
            ++mesh->nWeights;
            ++l->_nextMeshWeight;
            return true;
        };
        break; }
    // name
    case 'n': { mesh->name = g->strings->writeStr(str, len); break; }
    }
    return true;
}

bool GLTFLoader::handleMeshPrimitive(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    Gobj::Mesh * mesh = g->meshes + l->crumb(-3).index;
    Gobj::MeshPrimitive * prim = mesh->primitives + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (*(uint16_t *)c.key) {
    // attributes
    case 'a'|'t'<<8: {
        // set array location
        prim->attributes = g->meshAttributes + l->_nextMeshAttribute;
        // handle each attribute
        c.handleChild = [prim](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            Gobj::MeshAttribute * ma = prim->attributes + prim->nAttributes;
            ma->type = Gobj::attrFromStr(l->crumb().key);
            ma->accessor = g->accessors + Number{str, len};
            ++prim->nAttributes;
            ++l->_nextMeshAttribute;
            return true;
        };
        break; }
    // indices
    case 'i'|'n'<<8: { prim->indices = g->accessors + Number{str, len}; break; }
    // material
    case 'm'|'a'<<8: { prim->material = g->materials + Number{str, len}; break; }
    // mode
    case 'm'|'o'<<8: { prim->mode = (Gobj::MeshPrimitive::Mode)(int)Number{str, len}; break; }
    // targets
    case 't'|'a'<<8: {
        // set array location
        prim->targets = g->meshTargets + l->_nextMeshTarget;
        // handle each target
        c.handleChild = [prim](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            Gobj::MeshTarget * targ = prim->targets + prim->nTargets;
            targ->attributes = g->meshAttributes + l->_nextMeshAttribute;
            // handle each attribute of the target
            l->crumb().handleChild = [targ](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
                targ->attributes[targ->nAttributes].type = Gobj::attrFromStr(l->crumb().key);
                targ->attributes[targ->nAttributes].accessor = g->accessors + Number{str, len};
                ++targ->nAttributes;
                ++l->_nextMeshAttribute;
                return true;
            };
            ++prim->nTargets;
            ++l->_nextMeshTarget;
            return true;
        };
        break; }
    }
    return true;
}

bool GLTFLoader::handleNode(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Node * node = g->nodes + l->crumb(-1).index;
    switch (*(uint16_t *)c.key) {
    // camera
    case 'c'|'a'<<8: {
        node->camera = g->cameras + Number{str, len};
        break; }
    // children
    case 'c'|'h'<<8: {
        node->children = g->nodeChildren + l->_nextNodeChild;
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            node->children[node->nChildren] = g->nodes + Number{str, len};
            ++node->nChildren;
            ++l->_nextNodeChild;
            return true;
        };
        break; }
    // matrix
    case 'm'|'a'<<8: {
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            glm::value_ptr(node->matrix)[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // mesh
    case 'm'|'e'<<8: {
        node->mesh = g->meshes + Number{str, len};
        break; }
    // rotation
    case 'r'|'o'<<8: {
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            uint32_t i = l->crumb().index;
            // mind the order! glm quat can be wxyz or xyzw
            // see GLM_FORCE_QUAT_DATA_WXYZ
            // the `glm::quat` constructor ALWAYS takes:
            // w, x, y, z! w first!!!
            // https://stackoverflow.com/a/49416643/560384
            // (but we're not using the constructor here)
            #ifdef GLM_FORCE_QUAT_DATA_WXYZ
            node->rotation[(i+1)%4] = Number{str, len};
            #else
            node->rotation[i] = Number{str, len};
            #endif
            return true;
        };
        break; }
    // scale
    case 's'|'c'<<8: {
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            node->scale[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // skin
    case 's'|'k'<<8: {
        node->skin = g->skins + Number{str, len};
        break; }
    // translation
    case 't'|'r'<<8: {
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            node->translation[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // weights
    case 'w'|'e'<<8: {
        node->weights = g->nodeWeights + l->_nextNodeWeight;
        c.handleChild = [node](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            node->weights[l->crumb().index] = Number{str, len};
            ++node->nWeights;
            ++l->_nextNodeWeight;
            return true;
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        node->name = g->strings->writeStr(str, len);
        break; }
    }
    return true;
}

bool GLTFLoader::handleSampler(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    Gobj::Sampler * samp = g->samplers + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (c.key[0]) {
    case 'm': {
        switch (c.key[1]) {
        // magFilter
        case 'a': { samp->magFilter = (Gobj::Sampler::Filter)(int)Number{str, len}; break; }
        // minFilter
        case 'i': { samp->minFilter = (Gobj::Sampler::Filter)(int)Number{str, len}; break; }
        }
        break; }
    case 'w': {
        switch (c.key[4]) {
        // wrapS
        case 'S': { samp->wrapS = (Gobj::Sampler::Wrap)(int)Number{str, len}; break; }
        // wrapT
        case 'T': { samp->wrapT = (Gobj::Sampler::Wrap)(int)Number{str, len}; break; }
        }
        break; }
    // name
    case 'n': { samp->name = g->strings->writeStr(str, len); break; }
    }
    return true;
}

bool GLTFLoader::handleScene(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    auto & c = l->crumb();
    Gobj::Scene * scn = g->scenes + l->crumb(-1).index;
    switch (*(uint16_t *)c.key) {
    // nodes (root children)
    case 'n'|'o'<<8: {
        scn->nodes = g->nodeChildren + l->_nextNodeChild;
        c.handleChild = [scn](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            scn->nodes[scn->nNodes] = g->nodes + Number{str, len};
            ++scn->nNodes;
            ++l->_nextNodeChild;
            return true;
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        snprintf(scn->name, Gobj::Scene::NameSize, "%.*s", len, str);
        break; }
    }

    return true;
}

bool GLTFLoader::handleSkin(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    Gobj::Skin * skin = g->skins + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (c.key[0]) {
    // inverseBindMatrices
    case 'i': { skin->inverseBindMatrices = g->accessors + Number{str, len}; break; }
    // joints
    case 'j': {
        skin->joints = g->nodeChildren + l->_nextNodeChild;
        c.handleChild = [skin](GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
            skin->joints[skin->nJoints] = g->nodes + Number{str, len};
            ++skin->nJoints;
            ++l->_nextNodeChild;
            return true;
        };
        break; }
    // skeleton
    case 's': { skin->skeleton = g->nodes + Number{str, len}; break; }
    // name
    case 'n': { skin->name = g->strings->writeStr(str, len); break; }
    }
    return true;
}

bool GLTFLoader::handleTexture(GLTFLoader * l, Gobj * g, char const * str, uint32_t len) {
    Gobj::Texture * tex = g->textures + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (*(uint16_t *)c.key) {
    // sampler
    case 's'|'a'<<8: { tex->sampler = g->samplers + Number{str, len}; break; }
    // source
    case 's'|'o'<<8: { tex->source = g->images + Number{str, len}; break; }
    // name
    case 'n'|'a'<<8: { tex->name = g->strings->writeStr(str, len); break; }
    }
    return true;
}
