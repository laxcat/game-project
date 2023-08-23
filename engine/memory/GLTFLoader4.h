#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Gobj.h"

class GLTFLoader4 {
// CONSTANTS
public:
    static constexpr size_t MaxDepth = 8;
    static constexpr size_t MaxKeyLen = 32;
// TYPES
public:
    // rapidjson handler
    class Counter {
    public:
        Counter(GLTFLoader4 * loader);
        bool Null();
        bool Bool(bool b);
        bool Int(int i);
        bool Uint(unsigned i);
        bool Int64(int64_t i);
        bool Uint64(uint64_t i);
        bool Double(double d);
        bool String(char const * str, uint32_t length, bool copy);
        bool RawNumber(char const * str, uint32_t length, bool copy);
        bool StartObject();
        bool Key(char const * str, uint32_t length, bool copy);
        bool EndObject(uint32_t memberCount);
        bool StartArray();
        bool EndArray(uint32_t elementCount);
    public:
        GLTFLoader4 * loader;
    };

    // rapidjson handler
    class Scanner {
    public:
        Scanner(GLTFLoader4 * loader);
        bool Null();
        bool Bool(bool b);
        bool Int(int i);
        bool Uint(unsigned i);
        bool Int64(int64_t i);
        bool Uint64(uint64_t i);
        bool Double(double d);
        bool String(char const * str, uint32_t length, bool copy);
        bool RawNumber(char const * str, uint32_t length, bool copy);
        bool StartObject();
        bool Key(char const * str, uint32_t length, bool copy);
        bool EndObject(uint32_t memberCount);
        bool StartArray();
        bool EndArray(uint32_t elementCount);
    public:
        GLTFLoader4 * loader;
    };

    enum ObjType {
        TYPE_UNKNOWN,
        TYPE_OBJ,
        TYPE_ARR,
        TYPE_STR,
        TYPE_NULL,
        TYPE_BOOL,
        TYPE_UINT,
        TYPE_INT,
        TYPE_FLOAT,
    };

    // each obj/arr shift gets a new crumb.
    // the crumbs array (count of depth) holds a path back to root
    struct Crumb {
        ObjType objType = TYPE_UNKNOWN;
        char key[MaxKeyLen] = {'\0'};
        uint32_t index = UINT32_MAX; // index if is child of array.

        Crumb(ObjType objType, char const * key = nullptr);
        Crumb();

        void setKey(char const * key);
        // match key? null key matches any (no check, always true)
        bool matches(char const * key) const;
        // match obj type and (optionally) key?
        bool matches(ObjType objType, char const * key = nullptr) const;
        char const * objTypeStr() const;
        bool hasKey() const;
    };

// API
    GLTFLoader4(char const * jsonStr);
    // returns crumbs. offset = 0 is top of stack.
    // offset < 0 returns into stack, offset > 0 is invalid.
    Crumb & crumb(int offset = 0);

    void calculateSize();

    char * prettyJSON() const;
    void printBreadcrumbs() const;


// STORAGE
public:
    // is not retained. might not be null terminated.
    char const * jsonStr;
    Crumb crumbs[MaxDepth];
    uint32_t depth = 0;
    char key[MaxKeyLen] = {'\0'};
    uint32_t arrIndices[MaxDepth];
    uint32_t arrDepth;
    Gobj::Counts counts;

// INTERNALS
private:
    void push(ObjType objType);
    void pop();
};
