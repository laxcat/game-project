#include "Gobj.h"
#include "FrameStack.h"
#if DEBUG
#include "../dev/print.h"
#include "../MrManager.h"
#endif // DEBUG

Gobj::Gobj(Gobj::Counts const & counts) :
    counts(counts)
{}
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
void Gobj::print() const {
    ::print(printToFrameStack());
}

char * Gobj::printToFrameStack() const {
    assert(mm.frameStack && "Frame stack not initialized.");
    FrameStack & fs = *mm.frameStack;

    char * str = (char *)fs.dataHead();

    fs.formatPen("Accessor    %011p (%d)\n", accessors,          counts.accessors);
    fs.formatPen("Animation   %011p (%d)\n", animations,         counts.animations);
    fs.formatPen("AChannels   %011p (%d)\n", animationChannels,  counts.animationChannels);
    fs.formatPen("ASamplers   %011p (%d)\n", animationSamplers,  counts.animationSamplers);
    fs.formatPen("Buffer      %011p (%d)\n", buffers,            counts.buffers);
    fs.formatPen("BufferView  %011p (%d)\n", bufferViews,        counts.bufferViews);
    fs.formatPen("Camera      %011p (%d)\n", cameras,            counts.cameras);
    fs.formatPen("Image       %011p (%d)\n", images,             counts.images);
    fs.formatPen("Material    %011p (%d)\n", materials,          counts.materials);
    fs.formatPen("Mesh        %011p (%d)\n", meshes,             counts.meshes);
    fs.formatPen("Node        %011p (%d)\n", nodes,              counts.nodes);
    fs.formatPen("Sampler     %011p (%d)\n", samplers,           counts.samplers);
    fs.formatPen("Scene       %011p (%d)\n", scenes,             counts.scenes);
    fs.formatPen("Skin        %011p (%d)\n", skins,              counts.skins);
    fs.formatPen("Texture     %011p (%d)\n", textures,           counts.textures);

    fs.formatPen("scene index %d (", scene);
    if (scene == -1) {
        fs.formatPen("0");
    }
    else {
        fs.formatPen("%p", scenes + scene);
    }
    fs.formatPen(")\n");

    fs.formatPen("copyright:  %s\n", copyright);
    fs.formatPen("generator:  %s\n", generator);
    fs.formatPen("version:    %s\n", version);
    fs.formatPen("minVersion: %s\n", minVersion);

    fs.terminatePen();

    // printl("test[[[%s]]]", str);

    return str;
}
#endif // DEBUG
