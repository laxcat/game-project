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
        static constexpr size_t MaxKeyLen = 256;

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

    Gobj::Counts calcDataSize(char const * jsonStr);
    bool load(Gobj * g, char const * jsonStr);

};
