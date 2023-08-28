#include "GLTFLoader4.h"
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include "../common/string_utils.h"
#include "../MrManager.h"

// -------------------------------------------------------------------------- //
// COUNTER
// -------------------------------------------------------------------------- //

GLTFLoader4::Counter::Counter(GLTFLoader4 * loader) :
    loader(loader)
{}

bool GLTFLoader4::Counter::Null() { return true; }
bool GLTFLoader4::Counter::Bool(bool b) { return true; }
bool GLTFLoader4::Counter::Int(int i) { return true; }
bool GLTFLoader4::Counter::Int64(int64_t i) { return true; }
bool GLTFLoader4::Counter::Uint64(uint64_t i) { return true; }
bool GLTFLoader4::Counter::Double(double d) { return true; }
bool GLTFLoader4::Counter::RawNumber(char const * str, uint32_t length, bool copy) { return true; }

bool GLTFLoader4::Counter::Uint(unsigned i) {
    loader->push(TYPE_UINT);
    // loader->printBreadcrumbs();

    if (loader->depth >= 3 &&
        loader->crumb(-2).matches(TYPE_ARR, "buffers") &&
        loader->crumb().matches("byteLength")
    ) {
        loader->counts.buffersLen += i;
    }

    loader->pop();
    return true;
}

bool GLTFLoader4::Counter::String(char const * str, uint32_t length, bool copy) {
    loader->push(TYPE_STR);
    // loader->printBreadcrumbs();

    Crumb & thisCrumb = loader->crumb();
    // name strings
    if (loader->depth >= 3 &&
        loader->crumb(-2).matches(TYPE_ARR,
            "accessors|animations|buffers|bufferViews|cameras|images|materials|"
            "meshes|nodes|samplers|scenes|skins|textures") &&
        thisCrumb.matches("name")
    ) {
        loader->counts.allStrLen += length + 1;
    }

    // asset strings
    else if (
        loader->depth >= 2 &&
        loader->crumb(-1).matches(TYPE_OBJ, "asset") &&
        thisCrumb.matches("copyright|generator|version|minVersion")
    ) {
        loader->counts.allStrLen += length + 1;
    }

    loader->pop();
    return true;
}


bool GLTFLoader4::Counter::StartObject() {
    loader->push(TYPE_OBJ);
    // loader->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Counter::Key(char const * str, uint32_t length, bool copy) {
    snprintf(loader->key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader4::Counter::EndObject(uint32_t memberCount) {
    loader->pop();
    return true;
}

bool GLTFLoader4::Counter::StartArray() {
    loader->push(TYPE_ARR);
    loader->pushIndex();
    // loader->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Counter::EndArray(uint32_t elementCount) {
    Crumb & crumb = loader->crumb();
    // count the main sub-objects
    if (loader->depth == 2) {
        if      (crumb.matches("accessors"  )) { loader->counts.accessors   = elementCount; }
        else if (crumb.matches("animations" )) { loader->counts.animations  = elementCount; }
        else if (crumb.matches("buffers"    )) { loader->counts.buffers     = elementCount; }
        else if (crumb.matches("bufferViews")) { loader->counts.bufferViews = elementCount; }
        else if (crumb.matches("cameras"    )) { loader->counts.cameras     = elementCount; }
        else if (crumb.matches("images"     )) { loader->counts.images      = elementCount; }
        else if (crumb.matches("materials"  )) { loader->counts.materials   = elementCount; }
        else if (crumb.matches("meshes"     )) { loader->counts.meshes      = elementCount; }
        else if (crumb.matches("nodes"      )) { loader->counts.nodes       = elementCount; }
        else if (crumb.matches("samplers"   )) { loader->counts.samplers    = elementCount; }
        else if (crumb.matches("scenes"     )) { loader->counts.scenes      = elementCount; }
        else if (crumb.matches("skins"      )) { loader->counts.skins       = elementCount; }
        else if (crumb.matches("textures"   )) { loader->counts.textures    = elementCount; }
    }
    // scene nodes
    else if (loader->depth == 4 && crumb.matches("nodes") && loader->crumb(-2).matches("scenes")) {
        loader->counts.sceneNodes += elementCount;
    }
    // animation channels and samplers
    else if (loader->depth == 4 && loader->crumb(-2).matches("animations")) {
        if      (crumb.matches("channels")) { loader->counts.animationChannels += elementCount; }
        else if (crumb.matches("samplers")) { loader->counts.animationSamplers += elementCount; }
    }
    loader->pop();
    loader->popIndex();
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //

GLTFLoader4::Scanner::Scanner(GLTFLoader4 * loader, Gobj * gobj) :
    loader(loader),
    gobj(gobj)
{}

bool GLTFLoader4::Scanner::Null() {
    loader->push(TYPE_NULL);
    loader->printBreadcrumbs();
    loader->pop();
    return true;
}
bool GLTFLoader4::Scanner::Int(int i) {
    loader->push(TYPE_INT);
    loader->printBreadcrumbs();
    if      (loader->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = loader->crumb(-2).index;
        uint16_t minMaxIndex = loader->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %d (int)", accIndex, loader->crumb(-1).key, minMaxIndex, i);
        if      (loader->crumb(-1).matches("min")) { gobj->accessors[accIndex].min[minMaxIndex] = (float)i; }
        else if (loader->crumb(-1).matches("max")) { gobj->accessors[accIndex].max[minMaxIndex] = (float)i; }
    }
    loader->pop();
    return true;
}
bool GLTFLoader4::Scanner::Int64(int64_t i) {
    loader->push(TYPE_INT);
    loader->printBreadcrumbs();
    loader->pop();
    return true;
}
bool GLTFLoader4::Scanner::Uint64(uint64_t i) {
    loader->push(TYPE_UINT);
    loader->printBreadcrumbs();
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::RawNumber(char const * str, uint32_t length, bool copy) { return true; }

bool GLTFLoader4::Scanner::Bool(bool b) {
    loader->push(TYPE_BOOL);
    loader->printBreadcrumbs();
    if (loader->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = loader->crumb(-1).index;
        if     (loader->crumb().matches("normalized"))     { gobj->accessors[index].normalized = b; }
    }
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::Uint(unsigned i) {
    loader->push(TYPE_UINT);
    loader->printBreadcrumbs();
    if (loader->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = loader->crumb(-1).index;
        if      (loader->crumb().matches("bufferView"))     { gobj->accessors[index].bufferView = gobj->bufferViews + i; }
        else if (loader->crumb().matches("byteOffset"))     { gobj->accessors[index].byteOffset = i; }
        else if (loader->crumb().matches("componentType"))  { gobj->accessors[index].componentType = (Gobj::Accessor::ComponentType)i; }
        else if (loader->crumb().matches("normalized"))     { gobj->accessors[index].normalized = i; }
        else if (loader->crumb().matches("count"))          { gobj->accessors[index].count = i; }

    }
    else if (loader->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = loader->crumb(-2).index;
        uint16_t minMaxIndex = loader->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %d (uint)", accIndex, loader->crumb(-1).key, minMaxIndex, i);
        if      (loader->crumb(-1).matches("min")) { gobj->accessors[accIndex].min[minMaxIndex] = (float)i; }
        else if (loader->crumb(-1).matches("max")) { gobj->accessors[accIndex].max[minMaxIndex] = (float)i; }
    }
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::Double(double d) {
    loader->push(TYPE_FLOAT);
    loader->printBreadcrumbs();
    if      (loader->crumb(-3).matches(TYPE_ARR, "accessors")) {
        uint16_t accIndex = loader->crumb(-2).index;
        uint16_t minMaxIndex = loader->crumb().index;
        // printl("ACCESSOR[%d] %s[%d] = %f (float)", accIndex, loader->crumb(-1).key, minMaxIndex, d);
        if      (loader->crumb(-1).matches("min")) { gobj->accessors[accIndex].min[minMaxIndex] = (float)d; }
        else if (loader->crumb(-1).matches("max")) { gobj->accessors[accIndex].max[minMaxIndex] = (float)d; }
    }
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::String(char const * str, uint32_t length, bool copy) {
    loader->push(TYPE_STR);
    loader->printBreadcrumbs();
    // handle name strings
    if (loader->crumb().matches("name")) {
        char const * key = loader->crumb(-2).key;
        uint16_t index = loader->crumb(-1).index;
        if      (strEqu(key, "accessors"))  { gobj->accessors[index].name   = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "animations")) { gobj->animations[index].name  = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "buffers"))    { gobj->buffers[index].name     = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "bufferViews")){ gobj->bufferViews[index].name = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "cameras"))    { gobj->cameras[index].name     = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "images"))     { gobj->images[index].name      = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "materials"))  { gobj->materials[index].name   = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "meshes"))     { gobj->meshes[index].name      = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "nodes"))      { gobj->nodes[index].name       = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "samplers"))   { gobj->samplers[index].name    = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "scenes"))     { gobj->scenes[index].name      = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "skins"))      { gobj->skins[index].name       = gobj->strings->writeStr(str, length); }
        else if (strEqu(key, "textures"))   { gobj->textures[index].name    = gobj->strings->writeStr(str, length); }
    }
    else if (loader->crumb(-1).matches(TYPE_OBJ, "asset")) {
        if      (loader->crumb().matches("copyright"))  { gobj->copyright   = gobj->strings->writeStr(str, length); }
        else if (loader->crumb().matches("generator"))  { gobj->generator   = gobj->strings->writeStr(str, length); }
        else if (loader->crumb().matches("version"))    { gobj->version     = gobj->strings->writeStr(str, length); }
        else if (loader->crumb().matches("minVersion")) { gobj->minVersion  = gobj->strings->writeStr(str, length); }
    }
    else if (loader->crumb(-2).matches(TYPE_ARR, "accessors")) {
        uint16_t index = loader->crumb(-1).index;
        if     (loader->crumb().matches("type")) { gobj->accessors[index].type = loader->accessorTypeFromStr(str); }

    }
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::StartObject() {
    loader->push(TYPE_OBJ);
    loader->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Scanner::Key(char const * str, uint32_t length, bool copy) {
    snprintf(loader->key, MaxKeyLen, "%.*s", length, str);
    return true;
}

bool GLTFLoader4::Scanner::EndObject(uint32_t memberCount) {
    loader->pop();
    return true;
}

bool GLTFLoader4::Scanner::StartArray() {
    loader->push(TYPE_ARR);
    loader->pushIndex();
    loader->printBreadcrumbs();
    return true;
}

bool GLTFLoader4::Scanner::EndArray(uint32_t elementCount) {
    loader->pop();
    loader->popIndex();
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

GLTFLoader4::GLTFLoader4(char const * jsonStr) :
    jsonStr(jsonStr)
{
}

GLTFLoader4::Crumb & GLTFLoader4::crumb(int offset) {
    // out of bounds, return invalid crumb
    if (depth <= -offset || offset > 0) {
        return invalidCrumb;
    }
    return crumbs[depth-1+offset];
}

void GLTFLoader4::calculateSize() {
    Counter counter{this};
    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr);
    reader.Parse(ss, counter);
}

bool GLTFLoader4::load(Gobj * gobj) {
    Scanner scanner{this, gobj};
    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr);
    reader.Parse(ss, scanner);
    return true;
}

char * GLTFLoader4::prettyJSON() const {
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    rapidjson::Reader reader;
    auto ss = rapidjson::StringStream(jsonStr);
    reader.Parse(ss, writer);
    // printf("%s\n", sb.GetString());

    return mm.frameFormatStr("%s", sb.GetString());
}

void GLTFLoader4::printBreadcrumbs() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s,%d) > ", crumbs[i].objTypeStr(), crumbs[i].key, crumbs[i].index);
    }
    printl();
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
