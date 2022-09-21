#include "MemMan.h"
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>
#include "../common/codetimer.h"
#include "../dev/print.h"
#include "../MrManager.h"
#include "GLTF.h"
#include "JSONPrinter.h"
#include "GLTFSizeFinder.h"
#include "GLTFLoader2.h"

namespace gltfutil {
    void printGLTF(gltf::GLTF * g) {
        // using namespace gltf;

        printl("GLTF info block:");
        printl("Accessor    %011p (%d)", g->accessors, g->nAccessors);
        printl("Animation   %011p (%d)", g->animations, g->nAnimations);
        printl("AChannels   %011p (%d)", g->animationChannels, g->nAnimationChannels);
        printl("ASamplers   %011p (%d)", g->animationSamplers, g->nAnimationSamplers);
        printl("Buffer      %011p (%d)", g->buffers, g->nBuffers);
        printl("BufferView  %011p (%d)", g->bufferViews, g->nBufferViews);
        printl("Camera      %011p (%d)", g->cameras, g->nCameras);
        printl("Image       %011p (%d)", g->images, g->nImages);
        printl("Material    %011p (%d)", g->materials, g->nMaterials);
        printl("Mesh        %011p (%d)", g->meshes, g->nMeshes);
        printl("Node        %011p (%d)", g->nodes, g->nNodes);
        printl("Sampler     %011p (%d)", g->samplers, g->nSamplers);
        printl("Scene       %011p (%d)", g->scenes, g->nScenes);
        printl("Skin        %011p (%d)", g->skins, g->nSkins);
        printl("Texture     %011p (%d)", g->textures, g->nTextures);

        print("scene index %d (", g->scene);
        if (g->scene == -1) print("0"); else print("%p", g->scenes + g->scene);
        printl(")");

        printl("copyright:  %s", g->copyright);
        printl("generator:  %s", g->generator);
        printl("version:    %s", g->version);
        printl("minVersion: %s", g->minVersion);

        // test some specific things about CesiumMilkTruck.gltf
        // printl("animation channel count: %d", g->animations[0].nChannels);
    }

    Counter getGLTFDataSize(char const * jsonStr) {
        GLTFSizeFinder scanner;
        rapidjson::Reader reader;
        auto fs = rapidjson::StringStream(jsonStr);
        reader.Parse(fs, scanner);
        return scanner.counter();
    }

    bool load(byte_t * dst, size_t dstSize, char const * jsonStr) {
        // Counter counter;
        // bool loaded = false;

        // GLTFSizeFinder scanner;
        // rapidjson::Reader reader;
        // auto fs = rapidjson::StringStream(jsonStr);

        // reader.Parse(fs, scanner);
        // scanner.printStats();
        // return scanner.counter();

        // _loaded = false;
        // GLTFLoader2 scanner{dst, dstSize, counted};
        // rapidjson::Reader reader;
        // size_t bufSize = 1024*256;
        // char * buf = (char *)mm.frameStack->alloc(bufSize);
        // if (!buf) return 0;
        // auto fs = rapidjson::FileReadStream(externalFP, buf, bufSize);
        // reader.Parse(fs, scanner);
        // scanner.checkCounts();
        // _loaded = true;
        // return _loaded;
        return false;
    }
}
