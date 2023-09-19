#pragma once
#include <functional>
#include <stdint.h>
#include <stddef.h>
#include <rapidjson/reader.h>
#include "../common/debug_defines.h"
#include "Gobj.h"

/*

Loads GLTF files into Gobj.

Implemented:
    • opening/reading .glb files
    • opening/reading EMBEDED .gltf files
    • Calculating size
    • loading strings
    • loading accessors
    • loading animations and related sub-objects
    • loading buffers
    • loading bufferViews
    • loading cameras
    • loading images
    • loading materials
    • loading meshes and related sub-objects

NOT implemented yet:
    • external images
    • accessor.sparce
    • any extensions
    • any extras
    • loading anything not specifically mentioned in "Implemented" list

*/


class GLTFLoader {
// CONSTANTS
public:
    static constexpr size_t MaxDepth = 12;
    static constexpr size_t MaxKeyLen = 32;

    using HandlerFnSig = bool (GLTFLoader *, Gobj *, char const *, uint32_t);

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
    struct Counter {
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

        GLTFLoader * l;
    };

    // rapidjson handler
    // responsible for transfering data from JSON to Gobj
    struct Scanner {
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

        GLTFLoader * l;
        Gobj * g;
        // handles every value (Bool, String and RawNumber)
        bool Value(ObjType type, char const * str, uint32_t length);
    };

    // each obj/arr shift gets a new crumb.
    // the _crumbs stack (count of _depth) holds a path back to root
    struct Crumb {
        ObjType objType = TYPE_UNKNOWN;
        char key[MaxKeyLen] = {'\0'};
        uint32_t index = UINT32_MAX; // index if this is child of array
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

        std::function<bool(GLTFLoader *, Gobj * g, char const *, uint32_t)> handleChild = nullptr;
        std::function<bool(GLTFLoader *, Gobj * g, uint32_t)> handleEnd = nullptr;
    };

// PUBLIC INTERFACE
public:
    GLTFLoader(byte_t const * gltfData, char const * loadingDir = nullptr);

    size_t calculateSize();
    bool load(Gobj * gobj);

    Gobj::Counts counts() const;
    bool isGLB() const;
    uint32_t binDataSize() const;
    bool validData() const;

    #if DEBUG
    char const * prettyJSON(uint32_t * prettyJSONSize = nullptr);
    #endif // DEBUG

// STORAGE
private:
    // carries all info necessary to determine byte-size of Gobj
    Gobj::Counts _counts;
    // json string. is NOT copied. might not be null terminated.
    byte_t const * _gltfData;
    bool _isGLB = false;
    // loading dir
    char const * _loadingDir = nullptr;
    // crumb stack
    Crumb _crumbs[MaxDepth];
    uint32_t _depth = 0;
    Crumb _invalidCrumb;
    // key buffer, for most recent key encountered
    char _key[MaxKeyLen] = {'\0'};

    // json reading tools
    Counter _counter;
    Scanner _scanner;
    rapidjson::Reader _reader;

    // tracking vars during scanning
    uint16_t _nextAnimationChannel = 0;
    uint16_t _nextAnimationSampler = 0;
    uint16_t _nextMeshPrimitive = 0;
    uint16_t _nextMeshTarget = 0;
    uint16_t _nextMeshWeight = 0;
    uint16_t _nextMeshAttribute = 0;
    uint16_t _nextNodeChild = 0;
    uint16_t _nextNodeWeight = 0;
    byte_t * _nextRawDataPtr = nullptr;

// INTERNALS
private:
    // push/pop to crumb stack
    void push(ObjType objType);
    void pop();
    // returns crumb from stack. offset==0 is top of stack.
    // offset<0 returns into stack, offset>0 is invalid.
    Crumb & crumb(int offset = 0);
    // handles string that might be a uri OR data-stream
    size_t handleDataString(byte_t * dst, char const * str, size_t strLength);
    // do additional work after load
    void postLoad(Gobj * g);

    // data access
    uint32_t jsonStrSize() const;
    char const * jsonStr() const;
    byte_t const * binChunkStart() const;
    byte_t const * binData() const;
    static char const * objTypeStr(ObjType objType);

    // debug
    #if DEBUG
    void printBreadcrumbs() const;
    #endif // DEBUG

// HANDLERS
private:
    static bool handleRoot              (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleAccessor          (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleAsset             (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleAnimation         (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleAnimationChannel  (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleAnimationSampler  (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleBuffer            (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleBufferView        (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleCamera            (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleImage             (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleMaterial          (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleMesh              (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleMeshPrimitive     (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleNode              (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleSampler           (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleScene             (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleSkin              (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);
    static bool handleTexture           (GLTFLoader * l, Gobj * g, char const * str, uint32_t len);

};
