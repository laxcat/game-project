#pragma once
#include "GLTF.h"
#include <functional>
#include "../common/string_utils.h"

struct GLTFLoader2 {

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

        Crumb(ObjType objType, char const * key = nullptr) {
            this->objType = objType;
            setKey(key);
        }

        Crumb() {}

        void setKey(char const * key) {
            if (key) {
                snprintf(this->key, MaxKeyLen, "%s", key);
            }
            else {
                this->key[0] = '\0';
            }
        }

        char const * objTypeStr() const {
            return
                (objType == OBJ) ? "OBJ" :
                (objType == ARR) ? "ARR" :
                (objType == STR) ? "STR" :
                (objType == INT) ? "INT" :
                "UNKNOWN";
        }

        bool hasKey() const { return (key[0] != '\0'); }
    };

    // forms path back to root. array count is `depth`
    Crumb crumbs[MaxDepth];
    size_t depth = 0;
    char key[MaxKeyLen] = {'\0'};

    GLTFLoader2() {

    }

    // push crumb onto bread crumb stack
    void push(Crumb::ObjType objType) {
        assert(depth < MaxDepth);
        crumbs[depth].objType = objType;
        // copy and consume key
        crumbs[depth].setKey(key);
        key[0] = '\0';
        ++depth;
    }

    void pop() {
        --depth;
    }

    void captureKey(char const * key) {
        snprintf(this->key, MaxKeyLen, "%s", key);
    }

    bool Null  ()           { return true; }
    bool Bool  (bool b)     { return true; }
    bool Int   (int i)      { return true; }
    bool Int64 (int64_t  i) { return true; }
    bool Uint64(uint64_t u) { return true; }
    bool Double(double   d) { return true; }
    bool RawNumber(char const * str, size_t length, bool copy) { return true; }

    bool Uint(unsigned u) {
        push(Crumb::INT);

        pop();
        return true;
    }

    bool String(char const * str, size_t length, bool copy) {
        push(Crumb::STR);

        pop();
        return true;
    }

    bool Key(char const * str, size_t length, bool copy) {
        captureKey(str);
        return true;
    }

    bool StartObject() {
        push(Crumb::OBJ);
        // printBreadcrumb();
        return true;
    }

    bool StartArray () {
        push(Crumb::ARR);
        // printBreadcrumb();
        return true;
    }

    bool EndObject(size_t memberCount) {
        pop();
        return true;
    }

    bool EndArray (size_t elementCount) {
        pop();
        // printBreadcrumb();
        return true;
    }

    void printBreadcrumb() const {
        if (depth < 1) return;
        for (int i = 0; i < depth; ++i) {
            print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
        }
        printl();
    }
};
