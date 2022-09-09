#pragma once
#include "GLTF.h"
#include <stddef.h>

struct GLTFLoader2 {

    static constexpr size_t MaxDepth = 8;
    static constexpr size_t MaxKeyLen = 256;

    enum ObjType {
        TYPE_UNKNOWN,
        TYPE_OBJ,
        TYPE_ARR,
        TYPE_STR,
        TYPE_INT
    };

    // each obj/arr shift gets a new crumb.
    // the crumbs array (count of depth) holds a path back to root
    struct Crumb {
        ObjType objType = TYPE_UNKNOWN;
        char key[MaxKeyLen] = {'\0'};

        Crumb() {}
        
        Crumb(ObjType objType, char const * key = nullptr);
        void setKey(char const * key);
        // match key? null key matches any (no check, always true)
        bool matches(char const * key) const;
        // match obj type and (optionally) key?
        bool matches(ObjType objType, char const * key = nullptr) const;
        char const * objTypeStr() const;
        bool hasKey() const;
    };

    // forms path back to root. array count is `depth`
    Crumb crumbs[MaxDepth];
    size_t depth = 0;
    char key[MaxKeyLen] = {'\0'};
    byte_t * dst = nullptr;
    size_t dstSize = 0;
    size_t head = 0;
    gltf::GLTF * gltf;

    GLTFLoader2(byte_t * dst, size_t dstSize);
    void push(ObjType objType);
    void pop();
    void captureKey(char const * key);
    Crumb & crumbAt(int offset);

    bool Null  ();
    bool Bool  (bool b);
    bool Int   (int i);
    bool Int64 (int64_t  i);
    bool Uint64(uint64_t u);
    bool Double(double   d);
    bool RawNumber(char const * str, size_t length, bool copy);
    bool Uint(unsigned u);
    bool String(char const * str, size_t length, bool copy);
    bool Key(char const * str, size_t length, bool copy);
    bool StartObject();
    bool StartArray ();
    bool EndObject(size_t memberCount);
    bool EndArray (size_t elementCount);
    void printBreadcrumb() const;
};
