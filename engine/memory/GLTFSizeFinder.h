#pragma once
#include "GLTF.h"
#include <functional>
#include "../common/string_utils.h"

struct GLTFSizeFinder {

    static constexpr size_t MaxDepth = 8;
    static constexpr size_t MaxConditions = 16;
    static constexpr size_t MaxKeyLen = 16;

    using Action = std::function<void(void *)>;

    // each obj/arr shift gets a new crumb.
    // the crumbs array (count of depth) holds a path back to root
    struct Crumb {
        enum ObjType {
            UNKNOWN,
            OBJ,
            ARR,
            STR
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
                "UNKNOWN";
        }

        bool hasKey() const { return (key[0] != '\0'); }
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

        void push(int offset, Crumb::ObjType objType, char const * key = nullptr, Action action = nullptr) {
            assert(count < MaxConditions && "MaxConditions reached");
            auto & c = all[count];
            c.offset = offset;
            c.crumb.objType = objType;
            c.crumb.setKey(key);
            c.action = action;
            ++count;
        }
    };

    // forms path back to root. array count is `depth`
    Crumb crumbs[MaxDepth];
    size_t depth = 0;

    char key[MaxKeyLen] = {'\0'};
    char temp[MaxKeyLen] = {'\0'};
    ConditionGroup stringConds;
    ConditionGroup endArrayConds;

    uint16_t nAccessors = 0;
    uint16_t nAnimations = 0;
    uint16_t nBuffers = 0;
    uint16_t nBufferViews = 0;
    uint16_t nCameras = 0;
    uint16_t nImages = 0;
    uint16_t nMaterials = 0;
    uint16_t nMeshes = 0;
    uint16_t nNodes = 0;
    uint16_t nSamplers = 0;
    uint16_t nScenes = 0;
    uint16_t nSkins = 0;
    uint16_t nTextures = 0;

    uint32_t allStrLen = 0;

    GLTFSizeFinder() {
        endArrayConds.push(0, Crumb::ARR, "accessors", [this](void * ud){ nAccessors = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "animations", [this](void * ud){ nAnimations = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "buffers", [this](void * ud){ nBuffers = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "bufferViews", [this](void * ud){ nBufferViews = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "cameras", [this](void * ud){ nCameras = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "images", [this](void * ud){ nImages = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "materials", [this](void * ud){ nMaterials = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "meshes", [this](void * ud){ nMeshes = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "nodes", [this](void * ud){ nNodes = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "samplers", [this](void * ud){ nSamplers = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "scenes", [this](void * ud){ nScenes = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "skins", [this](void * ud){ nSkins = (size_t)ud; });
        endArrayConds.push(0, Crumb::ARR, "textures", [this](void * ud){ nTextures = (size_t)ud; });

        stringConds.allAction = [this](void * ud){
            allStrLen += (size_t)ud;
            printl("allStrLen %zu (added %zu)", allStrLen, (size_t)ud);
        };
        stringConds.push(0, Crumb::STR, "name");
        stringConds.push(-2, Crumb::ARR, "scenes");
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

    // check condition against current state
    bool check(Condition const & con, void * userData = nullptr) {
        // offset relative to last (most recent) crumb in stack
        auto & actualCrumb  = crumbs[depth-1+con.offset];

        printl("check %s against %s", con.crumb.objTypeStr(), actualCrumb.objTypeStr());
        if (con.crumb.objType != actualCrumb.objType) return false;
        printl("check (%s) against (%s)", con.crumb.key, actualCrumb.key);
        if (con.crumb.hasKey() && !strEqu(con.crumb.key, actualCrumb.key)) return false;
        if (con.action) con.action(userData);
        printl("TRUE!");
        return true;
    }

    // checks each condition in group (never short circuts).
    // returns true if ANY condition was true.
    bool checkEach(ConditionGroup const & group, void * userData = nullptr) {
        bool ret = false;
        for (int i = 0; i < group.count; ++i) {
            ret = ret || check(group.all[i], userData);
        }
        return ret;
    }

    // checks combination of all conditions in group (short circuts at first false).
    // returns true only if ALL conditions were true.
    bool checkAll(ConditionGroup const & group, void * userData = nullptr) {
        printl("check all user data: %zu", (size_t)userData);
        for (int i = 0; i < group.count; ++i) {
            if (!check(group.all[i], userData)) return false;
        }
        if (group.allAction) group.allAction(userData);
        return true;
    }

    size_t totalSize() const {
        return
            sizeof(gltf::GLTF) +
            sizeof(gltf::Accessor)   * nAccessors +
            sizeof(gltf::Accessor)   * nAccessors +
            sizeof(gltf::Animation)  * nAnimations +
            sizeof(gltf::Buffer)     * nBuffers +
            sizeof(gltf::BufferView) * nBufferViews +
            sizeof(gltf::Camera)     * nCameras +
            sizeof(gltf::Image)      * nImages +
            sizeof(gltf::Material)   * nMaterials +
            sizeof(gltf::Mesh)       * nMeshes +
            sizeof(gltf::Node)       * nNodes +
            sizeof(gltf::Sampler)    * nSamplers +
            sizeof(gltf::Scene)      * nScenes +
            sizeof(gltf::Skin)       * nSkins +
            sizeof(gltf::Texture)    * nTextures +
            allStrLen
        ;
    }

    bool Null  ()           { return true; }
    bool Bool  (bool b)     { return true; }
    bool Int   (int i)      { return true; }
    bool Uint  (unsigned u) { return true; }
    bool Int64 (int64_t  i) { return true; }
    bool Uint64(uint64_t u) { return true; }
    bool Double(double   d) { return true; }
    bool RawNumber(char const * str, size_t length, bool copy) { return true; }

    bool String(char const * str, size_t length, bool copy) {
        push(Crumb::STR);
        checkAll(stringConds, (void *)(length + 1));
        pop();
        return true;
    }

    bool Key(char const * str, size_t length, bool copy) {
        captureKey(str);
        return true;
    }

    bool StartObject() {
        push(Crumb::OBJ);
        printBreadcrumb();
        return true;
    }

    bool StartArray () {
        push(Crumb::ARR);
        printBreadcrumb();
        return true;
    }

    bool EndObject(size_t  memberCount) {
        pop();
        printBreadcrumb();
        return true;
    }

    bool EndArray (size_t elementCount) {
        checkEach(endArrayConds, (void *)elementCount);
        pop();
        printBreadcrumb();
        return true;
    }

    void printBreadcrumb() const {
        if (depth < 1) return;
        for (int i = 0; i < depth; ++i) {
            print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
        }
        printl();
    }

    void printStats() const {
        printl("Accessors:   %6u", nAccessors);
        printl("Animations:  %6u", nAnimations);
        printl("Buffers:     %6u", nBuffers);
        printl("BufferViews: %6u", nBufferViews);
        printl("Cameras:     %6u", nCameras);
        printl("Images:      %6u", nImages);
        printl("Materials:   %6u", nMaterials);
        printl("Meshes:      %6u", nMeshes);
        printl("Nodes:       %6u", nNodes);
        printl("Samplers:    %6u", nSamplers);
        printl("Scenes:      %6u", nScenes);
        printl("Skins:       %6u", nSkins);
        printl("Textures:    %6u", nTextures);
        printl("Strings len: %6u", allStrLen);
        printl("Total Size:%8zu", totalSize());
    }

};
