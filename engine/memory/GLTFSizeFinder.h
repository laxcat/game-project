#pragma once
#include "GLTF.h"
#include <functional>
#include "../common/string_utils.h"
#include "memutils.h"

struct GLTFSizeFinder {

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

    gltfutil::Counter cntr;

    int stringCounter = 0;

    GLTFSizeFinder() {
        objCountConds.push(0, Crumb::ARR, "accessors", [this](void * ud){ cntr.nAccessors = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "animations", [this](void * ud){ cntr.nAnimations = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "buffers", [this](void * ud){ cntr.nBuffers = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "bufferViews", [this](void * ud){ cntr.nBufferViews = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "cameras", [this](void * ud){ cntr.nCameras = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "images", [this](void * ud){ cntr.nImages = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "materials", [this](void * ud){ cntr.nMaterials = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "meshes", [this](void * ud){ cntr.nMeshes = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "nodes", [this](void * ud){ cntr.nNodes = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "samplers", [this](void * ud){ cntr.nSamplers = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "scenes", [this](void * ud){ cntr.nScenes = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "skins", [this](void * ud){ cntr.nSkins = (size_t)ud; });
        objCountConds.push(0, Crumb::ARR, "textures", [this](void * ud){ cntr.nTextures = (size_t)ud; });

        // animation sub objects
        animChannelCountConds.allAction = [this](void * ud) { cntr.nAnimationChannels += (size_t)ud; };
        animChannelCountConds.push( 0, Crumb::ARR, "channels");
        animChannelCountConds.push(-2, Crumb::ARR, "animations");
        animSamplerCountConds.allAction = [this](void * ud) { cntr.nAnimationSamplers += (size_t)ud; };
        animSamplerCountConds.push( 0, Crumb::ARR, "samplers");
        animSamplerCountConds.push(-2, Crumb::ARR, "animations");

        // mesh sub objects
        meshPrimCountConds.allAction = [this](void * ud) { cntr.nMeshPrimatives += (size_t)ud; };
        meshPrimCountConds.push( 0, Crumb::ARR, "primitives");
        meshPrimCountConds.push(-2, Crumb::ARR, "meshes");
        // TODO: add morph targets to same nMeshAttributes count
        meshAttrCountConds.allAction = [this](void * ud) { cntr.nMeshAttributes += (size_t)ud; };
        meshAttrCountConds.push( 0, Crumb::OBJ, "attributes");
        meshAttrCountConds.push(-2, Crumb::ARR, "primitives");
        meshAttrCountConds.push(-4, Crumb::ARR, "meshes");

        // find name strings
        // find main object names
        strCountConds[0].allAction = [this](void * ud) { ++stringCounter; cntr.allStrLen += (size_t)ud; };
        strCountConds[0].push(0, Crumb::STR, "name");
        strCountConds[0].push(
            -2,
            Crumb::ARR,
            "accessors|animations|buffers|bufferViews|cameras|images|materials|meshes|"
            "nodes|samplers|scenes|skins|textures"
        );
        // find asset names
        strCountConds[1].allAction = strCountConds[0].allAction;
        strCountConds[1].push(0, Crumb::STR, "copyright|generator|version|minVersion");
        strCountConds[1].push(-1, Crumb::OBJ, "asset");

        // find buffer sizes
        byteCountConds.allAction = [this](void * ud) { cntr.buffersLen += (size_t)ud; };
        byteCountConds.push(0, Crumb::INT, "byteLength");
        byteCountConds.push(-2, Crumb::ARR, "buffers");\


        // find image sizes
        // (unnecessary)
        // imageSizeConds.push(0, Crumb::STR, "uri");
        // imageSizeConds.push(-2, Crumb::ARR, "images");
        // imageSizeConds.allAction = [this](void * ud) {
        //     auto fs = (FixedStr *)ud;
        //     auto id = loadImageBase64(fs->str, fs->length);
        //     printl("image data: %dx%d, %d = %zu", id.width, id.height, id.nChannels, id.dataSize());
        // };
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

        // printl("check %s against %s", con.crumb.objTypeStr(), actualCrumb.objTypeStr());
        if (con.crumb.objType != actualCrumb.objType) return false;
        // printl("check (%s) against (%s)", con.crumb.key, actualCrumb.key);
        if (con.crumb.hasKey() && !strWithin(actualCrumb.key, con.crumb.key, '|')) return false;
        if (con.action) con.action(userData);
        // printl("TRUE!");
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
        // printl("check all user data: %zu", (size_t)userData);
        for (int i = 0; i < group.count; ++i) {
            if (!check(group.all[i], userData)) return false;
        }
        if (group.allAction) group.allAction(userData);
        return true;
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
        checkAll(byteCountConds, (void *)(size_t)u);
        pop();
        return true;
    }

    bool String(char const * str, size_t length, bool copy) {
        push(Crumb::STR);

        for (int i = 0; i < nFindStrConds; ++i) {
            checkAll(strCountConds[i], (void *)(ALIGN(length + 1)));
        }

        // special check for image size
        auto fs = FixedStr{str, length};
        checkAll(imageSizeConds, (void *)&fs);

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
        checkAll(meshAttrCountConds, (void *)memberCount);
        pop();
        return true;
    }

    bool EndArray (size_t elementCount) {
        checkEach(objCountConds, (void *)elementCount);
        checkAll(animChannelCountConds, (void *)elementCount);
        checkAll(animSamplerCountConds, (void *)elementCount);
        checkAll(meshPrimCountConds, (void *)elementCount);
        pop();
        // printBreadcrumb();
        return true;
    }

    gltfutil::Counter const & counter() const { return cntr; }

    size_t totalSize() const { return cntr.totalSize(); }

    void printBreadcrumb() const {
        if (depth < 1) return;
        for (int i = 0; i < depth; ++i) {
            print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
        }
        printl();
    }

    void printStats() const {
        printl("Accessors:    %8u", cntr.nAccessors);
        printl("Animations:   %8u", cntr.nAnimations);
        printl("Anim Channels:%8u", cntr.nAnimationChannels);
        printl("Anim Samplers:%8u", cntr.nAnimationSamplers);
        printl("Buffers:      %8u", cntr.nBuffers);
        printl("BufferViews:  %8u", cntr.nBufferViews);
        printl("Cameras:      %8u", cntr.nCameras);
        printl("Images:       %8u", cntr.nImages);
        printl("Materials:    %8u", cntr.nMaterials);
        printl("Meshes:       %8u", cntr.nMeshes);
        printl("Mesh prims:   %8u", cntr.nMeshPrimatives);
        printl("Mesh attrs:   %8u", cntr.nMeshAttributes);
        printl("Nodes:        %8u", cntr.nNodes);
        printl("Samplers:     %8u", cntr.nSamplers);
        printl("Scenes:       %8u", cntr.nScenes);
        printl("Skins:        %8u", cntr.nSkins);
        printl("Textures:     %8u", cntr.nTextures);
        printl("Strings len:  %8u", cntr.allStrLen);
        printl("String count: %8u", stringCounter);
        printl("Buffers len:  %8u", cntr.buffersLen);
        printl("Total Size:   %8zu", cntr.totalSize());
    }

};
