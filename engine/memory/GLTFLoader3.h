#pragma once
#include <functional>
#include "../dev/print.h"
#include "../common/string_utils.h"
#include "Gobj.h"

class GLTFLoader3 {
public:

    /*

    A rapidjson Handler that gathers the necessary info from the GLTF json to
    calculate byte-size of final game object.

    */

    struct SizeFinder {

        static constexpr size_t MaxDepth = 8;
        static constexpr size_t MaxConditions = 16;
        static constexpr size_t MaxKeyLen = 32;

        using Action = std::function<void(void *)>;

        // each obj/arr shift gets a new crumb.
        // the crumbs array (count of depth) holds a path back to root
        struct Crumb {
            enum ObjType {
                UNKNOWN,
                OBJ,
                ARR,
                STR,
                INT
            };

            ObjType objType = UNKNOWN;
            char key[MaxKeyLen] = {'\0'};

            Crumb(ObjType objType, char const * key = nullptr);
            Crumb();
            void setKey(char const * key);
            char const * objTypeStr() const;
            bool hasKey() const;
        };

        struct Condition {
            Crumb crumb;
            int offset;
            Action action = nullptr;
        };

        struct ConditionGroup {
            Condition all[MaxConditions];
            int count = 0;
            Action allAction = nullptr;

            void push(int offset, Crumb::ObjType objType, char const * key = nullptr, Action action = nullptr);
        };

        struct FixedStr {
            char const * str;
            size_t length;
        };

        // forms path back to root. array count is `depth`
        Crumb crumbs[MaxDepth];
        size_t depth = 0;
        char key[MaxKeyLen] = {'\0'};

        static constexpr int nFindStrConds = 2;
        ConditionGroup strCountConds[nFindStrConds];
        ConditionGroup objCountConds;
        ConditionGroup animChannelCountConds;
        ConditionGroup animSamplerCountConds;
        ConditionGroup meshPrimCountConds;
        ConditionGroup meshAttrCountConds;
        ConditionGroup byteCountConds;
        ConditionGroup imageSizeConds;

        Gobj::Counts cntr;

        int stringCounter = 0;

        SizeFinder();

        // push crumb onto bread crumb stack
        void push(Crumb::ObjType objType);
        void pop();

        void captureKey(char const * key);

        // check condition against current state
        bool check(Condition const & con, void * userData = nullptr);

        // checks each condition in group (never short circuts).
        // returns true if ANY condition was true.
        bool checkEach(ConditionGroup const & group, void * userData = nullptr);

        // checks combination of all conditions in group (short circuts at first false).
        // returns true only if ALL conditions were true.
        bool checkAll(ConditionGroup const & group, void * userData = nullptr);

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
        Gobj::Counts const & counter() const;
        size_t totalSize() const;
        void printBreadcrumb() const;
        void printStats() const;
    };

    /*

    A rapidjson Handler that loads GLTF into Gobj

    */

    struct Loader {

        static constexpr size_t MaxDepth = 8;
        static constexpr size_t MaxKeyLen = 32;

        enum ObjType {
            TYPE_UNKNOWN,
            TYPE_OBJ,
            TYPE_ARR,
            TYPE_STR,
            TYPE_UINT,
            TYPE_INT,
            TYPE_FLOAT,
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
        Gobj * gobj;
        FrameStack * strStack;
        Gobj::Counts const & counts;
        byte_t * buffers;

        Loader(byte_t * dst, size_t dstSize, Gobj::Counts const & counts);
        void push(ObjType objType);
        void pop();
        void captureKey(char const * key);
        Crumb & crumbAt(int offset);
        size_t handleData(byte_t * dst, char const * str, size_t strLength);

        bool Null  ();
        bool Bool  (bool b);
        bool Int   (int i);
        bool Int64 (int64_t  i);
        bool Uint64(uint64_t u);
        bool Double(double   d);
        bool RawNumber(char const * str, size_t length, bool copy);
        bool Uint(unsigned u);
        void handleNumber(void * any, ObjType hint);
        bool String(char const * str, size_t length, bool copy);
        bool Key(char const * str, size_t length, bool copy);
        bool StartObject();
        bool StartArray ();
        bool EndObject(size_t memberCount);
        bool EndArray (size_t elementCount);

        Gobj::Accessor::Type accessorTypeFromStr(char const * str);
        Gobj::AnimationTarget animationTargetFromStr(char const * str);
        Gobj::AnimationSampler::Interpolation interpolationFromStr(char const * str);
        Gobj::Camera::Type cameraTypeFromStr(char const * str);

        byte_t * head() const;
        // object heads (current)
        Gobj::Accessor * accessor() const;
        Gobj::Animation * animation() const;
        Gobj::AnimationChannel * animationChannel() const;
        Gobj::AnimationSampler * animationSampler() const;
        Gobj::Buffer * buffer() const;
        Gobj::BufferView * bufferView() const;
        Gobj::Camera * camera() const;

        void checkCounts() const;
        void printBreadcrumb() const;

    private:
        size_t _head = 0;
        byte_t * _dst = nullptr;
        size_t _dstSize = 0;

        float * _floatPtr = nullptr;
    };


    Gobj::Counts calcDataSize(char const * jsonStr);
    bool load(Gobj * g, size_t dstSize, Gobj::Counts const & counts, char const * jsonStr);

};
