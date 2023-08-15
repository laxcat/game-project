#include "Gobj.h"
#include "FrameStack.h"
#if DEBUG
#include "../dev/print.h"
#endif // DEBUG

byte_t const * Gobj::data () const { return (byte_t *)this + sizeof(Gobj); }
FrameStack const * Gobj::stringStack () const { return (FrameStack *)data(); }

size_t Gobj::Counts::totalSize() const {
    // mirrors memory order
    return
        sizeof(Gobj) +
        sizeof(FrameStack) + allStrLen +
        sizeof(Gobj::Accessor)          * accessors +
        sizeof(Gobj::Animation)         * animations +
        sizeof(Gobj::AnimationChannel)  * animationChannels +
        sizeof(Gobj::AnimationSampler)  * animationSamplers +
        sizeof(Gobj::Buffer)            * buffers +
        sizeof(Gobj::BufferView)        * bufferViews +
        sizeof(Gobj::Camera)            * cameras +
        sizeof(Gobj::Image)             * images +
        sizeof(Gobj::Material)          * materials +
        sizeof(Gobj::Mesh)              * meshes +
        sizeof(Gobj::MeshPrimative)     * meshPrimatives +
        sizeof(Gobj::MeshAttribute)     * meshAttributes +
        sizeof(Gobj::Node)              * nodes +
        sizeof(Gobj::Sampler)           * samplers +
        sizeof(Gobj::Scene)             * scenes +
        sizeof(Gobj::Skin)              * skins +
        sizeof(Gobj::Texture)           * textures +
        buffersLen
    ;
}

#if DEBUG
void Gobj::print(Gobj const * g) {
    // using namespace gltf;

    printl("Gobj info block:");
    printl("Accessor    %011p (%d)", g->accessors,          g->counts.accessors);
    printl("Animation   %011p (%d)", g->animations,         g->counts.animations);
    printl("AChannels   %011p (%d)", g->animationChannels,  g->counts.animationChannels);
    printl("ASamplers   %011p (%d)", g->animationSamplers,  g->counts.animationSamplers);
    printl("Buffer      %011p (%d)", g->buffers,            g->counts.buffers);
    printl("BufferView  %011p (%d)", g->bufferViews,        g->counts.bufferViews);
    printl("Camera      %011p (%d)", g->cameras,            g->counts.cameras);
    printl("Image       %011p (%d)", g->images,             g->counts.images);
    printl("Material    %011p (%d)", g->materials,          g->counts.materials);
    printl("Mesh        %011p (%d)", g->meshes,             g->counts.meshes);
    printl("Node        %011p (%d)", g->nodes,              g->counts.nodes);
    printl("Sampler     %011p (%d)", g->samplers,           g->counts.samplers);
    printl("Scene       %011p (%d)", g->scenes,             g->counts.scenes);
    printl("Skin        %011p (%d)", g->skins,              g->counts.skins);
    printl("Texture     %011p (%d)", g->textures,           g->counts.textures);

    ::print("scene index %d (", g->scene);
    if (g->scene == -1) {
        ::print("0");
    }
    else {
        ::print("%p", g->scenes + g->scene);
    }
    printl(")");

    printl("copyright:  %s", g->copyright);
    printl("generator:  %s", g->generator);
    printl("version:    %s", g->version);
    printl("minVersion: %s", g->minVersion);

    // test some specific things about CesiumMilkTruck.gltf
    // printl("animation channel count: %d", g->animations[0].nChannels);
}
#endif // DEBUG
