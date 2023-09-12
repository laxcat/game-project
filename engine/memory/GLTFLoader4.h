#pragma once
#include <functional>
#include <stdint.h>
#include <stddef.h>
#include "../common/debug_defines.h"
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
    • buffer uri (or any external file loading)
    • any extensions
    • any extras
    • loading anything not specifically mentioned in "Implemented" list

*/


class GLTFLoader4 {
// CONSTANTS
public:
    static constexpr size_t MaxDepth = 12;
    static constexpr size_t MaxKeyLen = 32;
// TYPES
public:
    // type of each SAX object while traversing JSON
    enum ObjType {
        TYPE_UNKNOWN,
        TYPE_OBJ,
        TYPE_ARR,
        TYPE_STR,
        TYPE_NUM,
    };

    // rapidjson handler
    // counts objects to determine size/layout of final Gobj
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
    // responsible for transfering data from JSON to Gobj
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
    private:
        GLTFLoader4 * l;
        Gobj * g;
        // handles both String and RawNumber
        bool Value(ObjType type, char const * str, uint32_t length);
    };

    // each obj/arr shift gets a new crumb.
    // the crumbs array (count of depth) holds a path back to root
    struct Crumb {
        ObjType objType = TYPE_UNKNOWN;
        char key[MaxKeyLen] = {'\0'};
        uint32_t index = UINT32_MAX; // index if is child of array.
        uint32_t childCount = 0;

        Crumb(ObjType objType, char const * key = nullptr);
        Crumb();

        void setKey(char const * key);
        // match key. null key matches any (no check, always true)
        bool matches(char const * key) const;
        // match obj type and (optionally) key
        bool matches(ObjType objType, char const * key = nullptr) const;
        char const * objTypeStr() const;
        bool hasKey() const;
        bool isValid() const;

        std::function<bool(GLTFLoader4 *, Gobj * g, char const *, uint32_t)> handleChild = nullptr;
        std::function<bool(GLTFLoader4 *, Gobj * g, uint32_t)> handleEnd = nullptr;
    };

// PUBLIC INTERFACE
public:
    GLTFLoader4(byte_t const * gltfData, char const * loadingDir = nullptr);
    // returns crumb from stack. offset==0 is top of stack.
    // offset<0 returns into stack, offset>0 is invalid.
    Crumb & crumb(int offset = 0);

    void calculateSize();
    bool load(Gobj * gobj);

    // data access
    uint32_t jsonStrSize() const;
    char const * jsonStr() const;
    byte_t const * binChunkStart() const;
    uint32_t binDataSize() const;
    byte_t const * binData() const;
    bool validData() const;
    static char const * objTypeStr(ObjType objType);

    char const * prettyJSON(uint32_t * prettyJSONSize = nullptr) const;
    void printBreadcrumbs() const;

// STORAGE
public:
    // jsonStr is not retained. might not be null terminated.
    byte_t const * gltfData;
    bool isGLB = false;
    // carries all info necessary to determine byte-size of Gobj
    Gobj::Counts counts;
    // loading dir
    char const * loadingDir = nullptr;
    // crumb stack
    Crumb crumbs[MaxDepth];
    uint32_t depth = 0;
    Crumb invalidCrumb;
    // key buffer, for most recent key encountered
    char key[MaxKeyLen] = {'\0'};

    uint16_t nextAnimationChannel = 0;
    uint16_t nextAnimationSampler = 0;
    uint16_t nextMeshPrimitive = 0;
    uint16_t nextMeshTarget = 0;
    uint16_t nextMeshWeight = 0;
    uint16_t nextMeshAttribute = 0;
    byte_t * nextRawDataPtr = nullptr;

// INTERNALS
private:
    // push/pop to crumb stack
    void push(ObjType objType);
    void pop();

// TODO: these conversion functions should be somewhere else maybe
public:
    Gobj::Accessor::Type accessorTypeFromStr(char const * str);
    Gobj::AnimationTarget animationTargetFromStr(char const * str);
    Gobj::AnimationSampler::Interpolation interpolationFromStr(char const * str);
    Gobj::Camera::Type cameraTypeFromStr(char const * str);
    Gobj::Image::MIMEType imageMIMETypeFromStr(char const * str);
    Gobj::Material::AlphaMode alphaModeFromStr(char const * str);
    Gobj::Attr attrFromStr(char const * str);
    size_t handleData(byte_t * dst, char const * str, size_t strLength);
};
