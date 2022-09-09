#include "GLTFLoader2.h"
#include <new>
#include <assert.h>
#include "GLTF.h"
#include "../common/string_utils.h"
#include "../dev/print.h"

#define C(OFFSET, ...) crumbAt(OFFSET).matches(__VA_ARGS__)

using namespace gltf;

GLTFLoader2::Crumb::Crumb(ObjType objType, char const * key) {
    this->objType = objType;
    setKey(key);
}

void GLTFLoader2::Crumb::setKey(char const * key) {
    if (key) {
        snprintf(this->key, MaxKeyLen, "%s", key);
    }
    else {
        this->key[0] = '\0';
    }
}

// match key? null key matches any (no check, always true)
bool GLTFLoader2::Crumb::matches(char const * key) const {
    if (!key) return true;
    return strWithin(this->key, key);
}

// match obj type and (optionally) key?
bool GLTFLoader2::Crumb::matches(ObjType objType, char const * key) const {
    if (objType != this->objType) return false;
    return matches(key);
}

char const * GLTFLoader2::Crumb::objTypeStr() const {
    return
        (objType == TYPE_OBJ) ? "OBJ" :
        (objType == TYPE_ARR) ? "ARR" :
        (objType == TYPE_STR) ? "STR" :
        (objType == TYPE_INT) ? "INT" :
        "UNKNOWN";
}

bool GLTFLoader2::Crumb::hasKey() const { return (key[0] != '\0'); }

GLTFLoader2::GLTFLoader2(byte_t * dst, size_t dstSize) :
    dst(dst),
    dstSize(dstSize)
{
    gltf = new (dst) GLTF{};
    head += sizeof(GLTF);
}

// push crumb onto bread crumb stack
void GLTFLoader2::push(ObjType objType) {
    assert(depth < MaxDepth);
    crumbs[depth].objType = objType;
    // copy and consume key
    crumbs[depth].setKey(key);
    key[0] = '\0';
    ++depth;
}

void GLTFLoader2::pop() {
    --depth;
}

void GLTFLoader2::captureKey(char const * key) {
    snprintf(this->key, MaxKeyLen, "%s", key);
}

GLTFLoader2::Crumb & GLTFLoader2::crumbAt(int offset) {
    assert(depth && 1-(int)depth <= offset && offset <= 0);
    return crumbs[depth-1+offset];
}

bool GLTFLoader2::Null  ()           { return true; }
bool GLTFLoader2::Bool  (bool b)     { return true; }
bool GLTFLoader2::Int   (int i)      { return true; }
bool GLTFLoader2::Int64 (int64_t  i) { return true; }
bool GLTFLoader2::Uint64(uint64_t u) { return true; }
bool GLTFLoader2::Double(double   d) { return true; }
bool GLTFLoader2::RawNumber(char const * str, size_t length, bool copy) { return true; }

bool GLTFLoader2::Uint(unsigned u) {
    push(TYPE_INT);

    pop();
    return true;
}

bool GLTFLoader2::String(char const * str, size_t length, bool copy) {
    push(TYPE_STR);

    pop();
    return true;
}

bool GLTFLoader2::Key(char const * str, size_t length, bool copy) {
    captureKey(str);
    return true;
}

bool GLTFLoader2::StartObject() {
    push(TYPE_OBJ);
    // printBreadcrumb();
    return true;
}

bool GLTFLoader2::StartArray () {
    push(TYPE_ARR);
    // printBreadcrumb();
    return true;
}

bool GLTFLoader2::EndObject(size_t memberCount) {
    pop();
    return true;
}

bool GLTFLoader2::EndArray (size_t elementCount) {
    // if (crumbAt(0).matches(TYPE_ARR, "meshes")) {
    if (C(0,TYPE_ARR,"")) {
        printl("FOUND %zu %s", elementCount, crumbAt(0).key);
    }
    pop();
    // printBreadcrumb();
    return true;
}

void GLTFLoader2::printBreadcrumb() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
    }
    printl();
}
