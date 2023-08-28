#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Gobj.h"

/*

Loads GLTF files into Gobj.

Implemented:
    • opening/reading .glb files
    • Calculating size
    • loading strings
    • loading accessors

NOT implemented yet:
    • opening/reading .gltf files
    • accessor.sparce
    • any extensions
    • any extras
    • loading anything not specifically mentioned in "Implemented" list

*/


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
        GLTFLoader4 * l;
    };

    // rapidjson handler
    class Scanner {
    public:
        Scanner(GLTFLoader4 * loader, Gobj * gobj);
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
        GLTFLoader4 * l;
        Gobj * g;
    private:
        uint16_t nextAnimationChannel = 0;
        uint16_t nextAnimationSampler = 0;
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
        // match key. null key matches any (no check, always true)
        bool matches(char const * key) const;
        // match obj type and (optionally) key
        bool matches(ObjType objType, char const * key = nullptr) const;
        char const * objTypeStr() const;
        bool hasKey() const;
    };

// API
    GLTFLoader4(char const * jsonStr);
    // returns crumb from stack. offset==0 is top of stack.
    // offset<0 returns into stack, offset>0 is invalid.
    Crumb & crumb(int offset = 0);

    void calculateSize();
    bool load(Gobj * gobj);

    char * prettyJSON() const;
    void printBreadcrumbs() const;


// STORAGE
public:
    // jsonStr is not retained. might not be null terminated.
    char const * jsonStr;
    // crumb stack
    Crumb crumbs[MaxDepth];
    uint32_t depth = 0;
    Crumb invalidCrumb;
    // most recent key encountered
    char key[MaxKeyLen] = {'\0'};
    // index stack for keeping track of index of array items
    uint32_t arrIndices[MaxDepth];
    uint32_t arrDepth;
    // carries all info necessary to determine byte-size of Gobj
    Gobj::Counts counts;

// INTERNALS
private:
    // push/pop to crumb stack
    void push(ObjType objType);
    void pop();
    // push/pop to index stack
    void pushIndex();
    void popIndex();

    Gobj::Accessor::Type accessorTypeFromStr(char const * str);
    Gobj::AnimationTarget animationTargetFromStr(char const * str);
    Gobj::AnimationSampler::Interpolation interpolationFromStr(char const * str);
};
