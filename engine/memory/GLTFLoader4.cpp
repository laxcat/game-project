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

#define PRINT_BREADCRUMBS 1

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
        strWithin(l->key, "copyright|generator|version|minVersion", '|')
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
    if (
        (l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches("primitives") &&
        l->crumb().matches("attributes"))
        ||
        (l->crumb(-5).matches(TYPE_ARR, "meshes") &&
        l->crumb(-3).matches("primitives") &&
        l->crumb(-1).matches("targets"))
    ) {
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
    // node children and scene children
    else if (
        l->depth == 4 &&
        ((crumb.matches("nodes") && l->crumb(-2).matches("scenes"))
        ||
        (crumb.matches("children") && l->crumb(-2).matches("nodes")))
    ) {
        l->counts.nodeChildren += elementCount;
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
    else if (l->crumb(-4).matches(TYPE_ARR, "meshes") &&
        l->crumb(-2).matches(TYPE_ARR, "primitives") &&
        l->crumb().matches(TYPE_ARR, "targets"))
    {
        l->counts.meshTargets += elementCount;
    }
    else if (l->depth == 4 && l->crumb(-2).matches("nodes") && crumb.matches("weights")) {
        l->counts.nodeWeights += elementCount;
    }
    l->pop();
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER HANDLER FORWARDS
// -------------------------------------------------------------------------- //

#define HANDLE_CHILD_SIG GLTFLoader4 * l, Gobj * g, char const * str, uint32_t len
#define HANDLE_END_SIG GLTFLoader4 * l, Gobj * g, uint32_t count
static bool handleRoot(HANDLE_CHILD_SIG);
static bool handleAccessor(HANDLE_CHILD_SIG);
static bool handleAsset(HANDLE_CHILD_SIG);
static bool handleAnimation(HANDLE_CHILD_SIG);
static bool handleAnimationChannel(HANDLE_CHILD_SIG);
static bool handleAnimationSampler(HANDLE_CHILD_SIG);
static bool handleBuffer(HANDLE_CHILD_SIG);
static bool handleBufferView(HANDLE_CHILD_SIG);
static bool handleCamera(HANDLE_CHILD_SIG);
static bool handleImage(HANDLE_CHILD_SIG);
static bool handleMaterial(HANDLE_CHILD_SIG);
static bool handleMesh(HANDLE_CHILD_SIG);
static bool handleMeshPrimitive(HANDLE_CHILD_SIG);
static bool handleNode(HANDLE_CHILD_SIG);
static bool handleSampler(HANDLE_CHILD_SIG);
static bool handleScene(HANDLE_CHILD_SIG);
static bool handleSkin(HANDLE_CHILD_SIG);

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //

GLTFLoader4::Scanner::Scanner(GLTFLoader4 * loader, Gobj * gobj) :
    l(loader),
    g(gobj)
{
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
    return Value(TYPE_NUM, b?"1":"0", 1);
}
bool GLTFLoader4::Scanner::RawNumber(char const * str, uint32_t length, bool copy) {
    return Value(TYPE_NUM, str, length);
}
bool GLTFLoader4::Scanner::String(char const * str, uint32_t length, bool copy) {
    return Value(TYPE_STR, str, length);
}

bool GLTFLoader4::Scanner::Value(ObjType type, char const * str, uint32_t length) {
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

bool GLTFLoader4::Scanner::StartObject() {
    l->push(TYPE_OBJ);

    // if has parent, run parent's handleChild
    if (l->crumb(-1).handleChild) {
        bool success = l->crumb(-1).handleChild(l, g, nullptr, 0);
        if (!success) return false;
    }
    // set new handle child
    if (l->depth == 1) {
        l->crumb().handleChild = handleRoot;
    }

    #if DEBUG && PRINT_BREADCRUMBS
    l->printBreadcrumbs();
    printl();
    #endif // DEBUG
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
        bool success = l->crumb(-1).handleChild(l, g, nullptr, 0);
        if (!success) return false;
    }
    #if DEBUG && PRINT_BREADCRUMBS
    l->printBreadcrumbs();
    printl();
    #endif // DEBUG
    return true;
}

bool GLTFLoader4::Scanner::EndArray(uint32_t elementCount) {
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

size_t GLTFLoader4::handleDataString(byte_t * dst, char const * str, size_t strLength) {
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

// -------------------------------------------------------------------------- //
// HANDLERS
// -------------------------------------------------------------------------- //

bool handleRoot(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    switch(*(uint16_t *)c.key) {
    // accessors
    case 'a'|'c'<<8: {
        printl("!!!!!!!! accessors");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleAccessor;
            return true;
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
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleAnimation;
            return true;
        };
        break; }
    // buffers|bufferViews
    case 'b'|'u'<<8: {
        switch(c.key[6]) {
        // buffers
        case 's': {
            printl("!!!!!!!! buffers");
            c.handleChild = [](HANDLE_CHILD_SIG) {
                l->crumb().handleChild = handleBuffer;
                return true;
            };
            break; }
        // bufferViews
        case 'V': {
            printl("!!!!!!!! bufferViews");
            c.handleChild = [](HANDLE_CHILD_SIG) {
                l->crumb().handleChild = handleBufferView;
                return true;
            };
            break; }
        }
        break; }
    // cameras
    case 'c'|'a'<<8: {
        printl("!!!!!!!! cameras");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleCamera;
            return true;
        };
        break; }
    // images
    case 'i'|'m'<<8: {
        printl("!!!!!!!! images");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleImage;
            return true;
        };
        break; }
    // materials
    case 'm'|'a'<<8: {
        printl("!!!!!!!! materials");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleMaterial;
            return true;
        };
        break; }
    // meshes
    case 'm'|'e'<<8: {
        printl("!!!!!!!! meshes");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleMesh;
            return true;
        };
        break; }
    // nodes
    case 'n'|'o'<<8: {
        printl("!!!!!!!! nodes");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleNode;
            return true;
        };
        break; }
    // samplers
    case 's'|'a'<<8: {
        printl("!!!!!!!! samplers");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleSampler;
            return true;
        };
        break; }
    // scenes
    case 's'|'c'<<8: {
        switch (c.key[5]) {
        case '\0': {
            printl("!!!!!!!! scene");
            g->scene = g->scenes + Number{str, len};
            break; }
        case 's': {
            printl("!!!!!!!! scenes");
            c.handleChild = [](HANDLE_CHILD_SIG) {
                l->crumb().handleChild = handleScene;
                return true;
            };
            break; }
        }
        break; }
    // skins
    case 's'|'k'<<8: {
        printl("!!!!!!!! skins");
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleSkin;
            return true;
        };
        break; }
    // textures
    case 't'|'e'<<8: { printl("!!!!!!!! textures"); break; }
    }
    return true;
}

bool handleAccessor(HANDLE_CHILD_SIG) {
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
        c.handleChild = [](HANDLE_CHILD_SIG) {
            uint16_t accIndex = l->crumb(-2).index;
            uint16_t maxIndex = l->crumb().index;
            g->accessors[accIndex].max[maxIndex] = Number{str, len};
            return true;
        };
        break; }
    // min
    case 'm'|'i'<<8: {
        c.handleChild = [](HANDLE_CHILD_SIG) {
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

bool handleAsset(HANDLE_CHILD_SIG) {
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

bool handleAnimation(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    uint16_t aniIndex = l->crumb(-1).index;
    switch(*(uint16_t *)c.key) {
    // channels
    case 'c'|'h'<<8: {
        g->animations[aniIndex].channels = g->animationChannels + l->nextAnimationChannel;
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleAnimationChannel;
            return true;
        };
        c.handleEnd = [aniIndex](HANDLE_END_SIG) {
            g->animations[aniIndex].nChannels = count;
            l->nextAnimationChannel += count;
            return true;
        };
        break; }
    // samplers
    case 's'|'a'<<8: {
        g->animations[aniIndex].samplers = g->animationSamplers + l->nextAnimationSampler;
        c.handleChild = [](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleAnimationSampler;
            return true;
        };
        c.handleEnd = [aniIndex](HANDLE_END_SIG) {
            g->animations[aniIndex].nSamplers = count;
            l->nextAnimationSampler += count;
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

bool handleAnimationChannel(HANDLE_CHILD_SIG) {
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
        c.handleChild = [](HANDLE_CHILD_SIG) {
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

bool handleAnimationSampler(HANDLE_CHILD_SIG) {
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

bool handleBuffer(HANDLE_CHILD_SIG) {
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
        size_t bytesWritten = l->handleDataString(l->nextRawDataPtr, str, len);
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

bool handleBufferView(HANDLE_CHILD_SIG) {
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

bool handleCamera(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    Gobj::Camera * cam = g->cameras + l->crumb(-1).index;
    switch (c.key[0]) {
    // orthographic
    case 'o': {
        cam->orthographic = (Gobj::CameraOrthographic *)&cam->_data;
        c.handleChild = [cam](HANDLE_CHILD_SIG) {
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
        c.handleChild = [cam](HANDLE_CHILD_SIG) {
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

bool handleImage(HANDLE_CHILD_SIG) {
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

bool handleMaterial(HANDLE_CHILD_SIG) {
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
            c.handleChild = [mat](HANDLE_CHILD_SIG) {
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
            c.handleChild = [mat](HANDLE_CHILD_SIG) {
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
            c.handleChild = [mat](HANDLE_CHILD_SIG) {
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
        c.handleChild = [mat](HANDLE_CHILD_SIG) {
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
        c.handleChild = [mat](HANDLE_CHILD_SIG) {
            switch (l->crumb().key[0]) {
            // baseColorFactor || baseColorTexture
            case 'b': {
                switch (l->crumb().key[9]) {
                // baseColorFactor
                case 'F': {
                    l->crumb().handleChild = [mat](HANDLE_CHILD_SIG) {
                        mat->baseColorFactor[l->crumb().index] = Number{str, len};
                        return true;
                    };
                    break; }
                // baseColorTexture
                case 'T': {
                    l->crumb().handleChild = [mat](HANDLE_CHILD_SIG) {
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
                    l->crumb().handleChild = [mat](HANDLE_CHILD_SIG) {
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

bool handleMesh(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    Gobj::Mesh * mesh = g->meshes + l->crumb(-1).index;
    switch (c.key[0]) {
    // primitives
    case 'p': {
        // primitives array
        mesh->primitives = g->meshPrimitives + l->nextMeshPrimitive;
        // handle each primitive
        c.handleChild = [mesh](HANDLE_CHILD_SIG) {
            l->crumb().handleChild = handleMeshPrimitive;
            return true;
        };
        // on end of primitives array
        c.handleEnd = [mesh](HANDLE_END_SIG) {
            mesh->nPrimitives = count;
            l->nextMeshPrimitive += count;
            return true;
        };
        break; }
    // weights
    case 'w': {
        mesh->weights = g->meshWeights + l->nextMeshWeight;
        c.handleChild = [mesh](HANDLE_CHILD_SIG) {
            mesh->weights[mesh->nWeights] = Number{str, len};
            ++mesh->nWeights;
            ++l->nextMeshWeight;
            return true;
        };
        break; }
    // name
    case 'n': { mesh->name = g->strings->writeStr(str, len); break; }
    }
    return true;
}

bool handleMeshPrimitive(HANDLE_CHILD_SIG) {
    Gobj::Mesh * mesh = g->meshes + l->crumb(-3).index;
    Gobj::MeshPrimitive * prim = mesh->primitives + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (l->crumb().key[0]) {
    // attributes
    case 'a': {
        // set array location
        prim->attributes = g->meshAttributes + l->nextMeshAttribute;
        // handle each attribute
        c.handleChild = [prim](HANDLE_CHILD_SIG) {
            prim->attributes[prim->nAttributes].type = Gobj::attrFromStr(l->crumb().key);
            prim->attributes[prim->nAttributes].accessor = g->accessors + Number{str, len};
            ++prim->nAttributes;
            ++l->nextMeshAttribute;
            return true;
        };
        break; }
    // indices
    case 'i': { prim->indices = g->accessors + Number{str, len}; break; }
    // material || mode
    case 'm': {
        switch (l->crumb().key[1]) {
        // material
        case 'a': { prim->material = g->materials + Number{str, len}; break; }
        // mode
        case 'o': { prim->mode = (Gobj::MeshPrimitive::Mode)(int)Number{str, len}; break; }
        }
        break; }
    // targets
    case 't': {
        // set array location
        prim->targets = g->meshTargets + l->nextMeshTarget;
        // handle each target
        c.handleChild = [prim](HANDLE_CHILD_SIG) {
            Gobj::MeshTarget * targ = prim->targets + prim->nTargets;
            targ->attributes = g->meshAttributes + l->nextMeshAttribute;
            // handle each attribute of the target
            l->crumb().handleChild = [targ](HANDLE_CHILD_SIG) {
                targ->attributes[targ->nAttributes].type = Gobj::attrFromStr(l->crumb().key);
                targ->attributes[targ->nAttributes].accessor = g->accessors + Number{str, len};
                ++targ->nAttributes;
                ++l->nextMeshAttribute;
                return true;
            };
            ++prim->nTargets;
            ++l->nextMeshTarget;
            return true;
        };
        break; }
    }
    return true;
}

bool handleNode(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    Gobj::Node * node = g->nodes + l->crumb(-1).index;
    switch (*(uint16_t *)c.key) {
    // camera
    case 'c'|'a'<<8: {
        node->camera = g->cameras + Number{str, len};
        break; }
    // children
    case 'c'|'h'<<8: {
        node->children = g->nodeChildren + l->nextNodeChild;
        c.handleChild = [node](HANDLE_CHILD_SIG) {
            node->children[node->nChildren] = g->nodes + Number{str, len};
            ++node->nChildren;
            ++l->nextNodeChild;
            return true;
        };
        break; }
    // matrix
    case 'm'|'a'<<8: {
        c.handleChild = [node](HANDLE_CHILD_SIG) {
            node->matrix[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // mesh
    case 'm'|'e'<<8: {
        node->mesh = g->meshes + Number{str, len};
        break; }
    // rotation
    case 'r'|'o'<<8: {
        c.handleChild = [node](HANDLE_CHILD_SIG) {
            node->rotation[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // scale
    case 's'|'c'<<8: {
        c.handleChild = [node](HANDLE_CHILD_SIG) {
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
        c.handleChild = [node](HANDLE_CHILD_SIG) {
            node->translation[l->crumb().index] = Number{str, len};
            return true;
        };
        break; }
    // weights
    case 'w'|'e'<<8: {
        node->weights = g->nodeWeights + l->nextNodeWeight;
        c.handleChild = [node](HANDLE_CHILD_SIG) {
            node->weights[l->crumb().index] = Number{str, len};
            ++node->nWeights;
            ++l->nextNodeWeight;
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

bool handleSampler(HANDLE_CHILD_SIG) {
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

bool handleScene(HANDLE_CHILD_SIG) {
    auto & c = l->crumb();
    Gobj::Scene * scn = g->scenes + l->crumb(-1).index;
    switch (*(uint16_t *)c.key) {
    // nodes (root children)
    case 'n'|'o'<<8: {
        scn->nodes = g->nodeChildren + l->nextNodeChild;
        c.handleChild = [scn](HANDLE_CHILD_SIG) {
            scn->nodes[scn->nNodes] = g->nodes + Number{str, len};
            ++scn->nNodes;
            ++l->nextNodeChild;
            return true;
        };
        break; }
    // name
    case 'n'|'a'<<8: {
        scn->name = g->strings->writeStr(str, len);
        break; }
    }

    return true;
}

bool handleSkin(HANDLE_CHILD_SIG) {
    Gobj::Skin * skin = g->skins + l->crumb(-1).index;
    auto & c = l->crumb();
    switch (c.key[0]) {
    // inverseBindMatrices
    case 'i': { skin->inverseBindMatrices = g->accessors + Number{str, len}; break; }
    // joints
    case 'j': {
        skin->joints = g->nodeChildren + l->nextNodeChild;
        c.handleChild = [skin](HANDLE_CHILD_SIG) {
            skin->joints[skin->nJoints] = g->nodes + Number{str, len};
            ++skin->nJoints;
            ++l->nextNodeChild;
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

/*

        else if (strEqu(key, "textures"))   { g->textures[index].name    = g->strings->writeStr(str, length); }

*/
