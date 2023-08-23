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

bool GLTFLoader4::Counter::Null() { printl("HOG DONKEY!1"); return true; }
bool GLTFLoader4::Counter::Bool(bool b) { printl("HOG DONKEY!2"); return true; }
bool GLTFLoader4::Counter::Int(int i) { printl("HOG DONKEY!3"); return true; }
bool GLTFLoader4::Counter::Int64(int64_t i) { printl("HOG DONKEY!4"); return true; }
bool GLTFLoader4::Counter::Uint64(uint64_t i) { printl("HOG DONKEY!5"); return true; }
bool GLTFLoader4::Counter::Double(double d) { return true; }
bool GLTFLoader4::Counter::RawNumber(char const * str, uint32_t length, bool copy) { printl("HOG DONKEY!7"); return true; }

bool GLTFLoader4::Counter::Uint(unsigned i) {
    loader->push(TYPE_UINT);
    loader->printBreadcrumbs();

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
    loader->printBreadcrumbs();

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
    loader->printBreadcrumbs();
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
    loader->printBreadcrumbs();
    loader->arrIndices[loader->arrDepth] = 0;
    ++loader->arrDepth;
    return true;
}

bool GLTFLoader4::Counter::EndArray(uint32_t elementCount) {
    Crumb & crumb = loader->crumbs[loader->depth-1];
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
    loader->pop();
    --loader->arrDepth;
    return true;
}

// -------------------------------------------------------------------------- //
// SCANNER
// -------------------------------------------------------------------------- //


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

// match key?
bool GLTFLoader4::Crumb::matches(char const * key) const {
    // passed null key matches any (no check, always true)
    if (!key || key[0] == '\0') return true;
    // always false if Crumb has no key, except true when empty string is passed
    if (!hasKey()) return strEqu(key, "");
    // both passed and Crumb key exists, check if passed key contains Crumb key
    return strWithin(this->key, key, '|');
}

// match obj type and (optionally) key?
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
    assert(depth &&
        offset <= 0 &&
        depth > -offset &&
        "Invalid offset."
    );
    return crumbs[depth-1+offset];
}

void GLTFLoader4::push(ObjType objType) {
    assert(depth < MaxDepth);

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

void GLTFLoader4::calculateSize() {
    Counter counter{this};
    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, counter);
}

char * GLTFLoader4::prettyJSON() const {
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, writer);
    // printf("%s\n", sb.GetString());

    return mm.frameFormatStr("%s", sb.GetString());

    return nullptr;
}

void GLTFLoader4::printBreadcrumbs() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s,%d) > ", crumbs[i].objTypeStr(), crumbs[i].key, crumbs[i].index);
    }
    printl();
}
