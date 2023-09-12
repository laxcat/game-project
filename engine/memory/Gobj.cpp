#include "Gobj.h"
#include <new>
#include "FrameStack.h"
#include "mem_utils.h"
#if DEBUG || DEV_INTERFACE
#include "../dev/print.h"
#include "../MrManager.h"
#endif // DEBUG || DEV_INTERFACE

#define ALIGN_SIZE(SIZE) alignSize(SIZE, Gobj::Align)
#define ALIGN_PTR(PTR)  alignPtr(PTR, Gobj::Align)

Gobj::Gobj(MemMan *, Gobj::Counts const & counts) :
    counts(counts)
{
    // don't align base head, we assume Gobj has been aligned externally (MemMan)
    byte_t * head = (byte_t *)this;
    head += ALIGN_SIZE(sizeof(Gobj));

    // has string data, create FrameStack to hold it
    if (counts.allStrLen) {
        strings = new (head) FrameStack{counts.allStrLen};
        head += ALIGN_SIZE(strings->totalSize());
    }

    if (counts.accessors) {
        accessors = (Accessor *)head;
        for (uint16_t i = 0; i < counts.accessors; ++i) {
            new (head) Accessor{};
            head += ALIGN_SIZE(sizeof(Accessor));
        }
    }

    if (counts.animations) {
        animations = (Animation *)head;
        for (uint16_t i = 0; i < counts.animations; ++i) {
            new (head) Animation{};
            head += ALIGN_SIZE(sizeof(Animation));
        }
    }

    if (counts.animationChannels) {
        animationChannels = (AnimationChannel *)head;
        for (uint16_t i = 0; i < counts.animationChannels; ++i) {
            new (head) AnimationChannel{};
            head += ALIGN_SIZE(sizeof(AnimationChannel));
        }
    }

    if (counts.animationSamplers) {
        animationSamplers = (AnimationSampler *)head;
        for (uint16_t i = 0; i < counts.animationSamplers; ++i) {
            new (head) AnimationSampler{};
            head += ALIGN_SIZE(sizeof(AnimationSampler));
        }
    }

    if (counts.buffers) {
        buffers = (Buffer *)head;
        for (uint16_t i = 0; i < counts.buffers; ++i) {
            new (head) Buffer{};
            head += ALIGN_SIZE(sizeof(Buffer));
        }
    }

    if (counts.bufferViews) {
        bufferViews = (BufferView *)head;
        for (uint16_t i = 0; i < counts.bufferViews; ++i) {
            new (head) BufferView{};
            head += ALIGN_SIZE(sizeof(BufferView));
        }
    }

    if (counts.cameras) {
        cameras = (Camera *)head;
        for (uint16_t i = 0; i < counts.cameras; ++i) {
            new (head) Camera{};
            head += ALIGN_SIZE(sizeof(Camera));
        }
    }

    if (counts.images) {
        images = (Image *)head;
        for (uint16_t i = 0; i < counts.images; ++i) {
            new (head) Image{};
            head += ALIGN_SIZE(sizeof(Image));
        }
    }

    if (counts.materials) {
        materials = (Material *)head;
        for (uint16_t i = 0; i < counts.materials; ++i) {
            new (head) Material{};
            head += ALIGN_SIZE(sizeof(Material));
        }
    }

    if (counts.meshes) {
        meshes = (Mesh *)head;
        for (uint16_t i = 0; i < counts.meshes; ++i) {
            new (head) Mesh{};
            head += ALIGN_SIZE(sizeof(Mesh));
        }
    }

    if (counts.meshAttributes) {
        meshAttributes = (MeshAttribute *)head;
        for (uint16_t i = 0; i < counts.meshAttributes; ++i) {
            new (head) MeshAttribute{};
            head += ALIGN_SIZE(sizeof(MeshAttribute));
        }
    }

    if (counts.meshPrimitives) {
        meshPrimitives = (MeshPrimitive *)head;
        for (uint16_t i = 0; i < counts.meshPrimitives; ++i) {
            new (head) MeshPrimitive{};
            head += ALIGN_SIZE(sizeof(MeshPrimitive));
        }
    }

    if (counts.meshTargets) {
        meshTargets = (MeshTarget *)head;
        for (uint16_t i = 0; i < counts.meshTargets; ++i) {
            new (head) MeshTarget{};
            head += ALIGN_SIZE(sizeof(MeshTarget));
        }
    }

    if (counts.meshWeights) {
        meshWeights = (float *)head;
        for (uint16_t i = 0; i < counts.meshWeights; ++i) {
            *(float *)head = 0.f;
            head += ALIGN_SIZE(sizeof(float));
        }
    }

    if (counts.nodes) {
        nodes = (Node *)head;
        for (uint16_t i = 0; i < counts.nodes; ++i) {
            new (head) Node{};
            head += ALIGN_SIZE(sizeof(Node));
        }
    }

    if (counts.samplers) {
        samplers = (Sampler *)head;
        for (uint16_t i = 0; i < counts.samplers; ++i) {
            new (head) Sampler{};
            head += ALIGN_SIZE(sizeof(Sampler));
        }
    }

    if (counts.scenes) {
        scenes = (Scene *)head;
        for (uint16_t i = 0; i < counts.scenes; ++i) {
            new (head) Scene{};
            head += ALIGN_SIZE(sizeof(Scene));
        }
    }

    if (counts.sceneNodes) {
        sceneNodes = (Node **)head;
        for (uint16_t i = 0; i < counts.sceneNodes; ++i) {
            *(Node **)head = nullptr;
            head += sizeof(Node *);
        }
        // aligns on group, not on each ptr
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.skins) {
        skins = (Skin *)head;
        for (uint16_t i = 0; i < counts.skins; ++i) {
            new (head) Skin{};
            head += ALIGN_SIZE(sizeof(Skin));
        }
    }

    if (counts.textures) {
        textures = (Texture *)head;
        for (uint16_t i = 0; i < counts.textures; ++i) {
            new (head) Texture{};
            head += ALIGN_SIZE(sizeof(Texture));
        }
    }

    if (counts.buffers) {
        rawData = head;
    }
}

byte_t const * Gobj::data () const { return (byte_t *)this + sizeof(Gobj); }
FrameStack const * Gobj::stringStack () const { return (FrameStack *)data(); }

size_t Gobj::Counts::totalSize() const {
    return
        ALIGN_SIZE(sizeof(Gobj)) +
        ALIGN_SIZE(sizeof(FrameStack) + allStrLen) +
        ALIGN_SIZE(sizeof(Gobj::Accessor))         * accessors +
        ALIGN_SIZE(sizeof(Gobj::Animation))        * animations +
        ALIGN_SIZE(sizeof(Gobj::AnimationChannel)) * animationChannels +
        ALIGN_SIZE(sizeof(Gobj::AnimationSampler)) * animationSamplers +
        ALIGN_SIZE(sizeof(Gobj::Buffer))           * buffers +
        ALIGN_SIZE(sizeof(Gobj::BufferView))       * bufferViews +
        ALIGN_SIZE(sizeof(Gobj::Camera))           * cameras +
        ALIGN_SIZE(sizeof(Gobj::Image))            * images +
        ALIGN_SIZE(sizeof(Gobj::Material))         * materials +
        ALIGN_SIZE(sizeof(Gobj::Mesh) )            * meshes +
        ALIGN_SIZE(sizeof(Gobj::MeshAttribute))    * meshAttributes +
        ALIGN_SIZE(sizeof(Gobj::MeshPrimitive))    * meshPrimitives +
        ALIGN_SIZE(sizeof(Gobj::MeshTarget))       * meshTargets +
        ALIGN_SIZE(sizeof(float))                  * meshWeights +
        ALIGN_SIZE(sizeof(Gobj::Node))             * nodes +
        ALIGN_SIZE(sizeof(Gobj::Sampler))          * samplers +
        ALIGN_SIZE(sizeof(Gobj::Scene))            * scenes +
        ALIGN_SIZE(sizeof(Gobj::Node *)            * sceneNodes) + // aligns on group, not on each ptr
        ALIGN_SIZE(sizeof(Gobj::Skin))             * skins +
        ALIGN_SIZE(sizeof(Gobj::Texture))          * textures +
        ALIGN_SIZE(rawDataLen)
    ;
}

#if DEBUG || DEV_INTERFACE

void Gobj::print() const {
    ::print(printToFrameStack());
}

char * Gobj::printToFrameStack() const {
    assert(mm.frameStack && "Frame stack not initialized.");
    FrameStack & fs = *mm.frameStack;

    char * str = (char *)fs.dataHead();

    fs.formatPen("Accessors   %011p (%d)\n", accessors,         counts.accessors);
    fs.formatPen("Animation   %011p (%d)\n", animations,        counts.animations);
    fs.formatPen("AChannels               (%d)\n",              counts.animationChannels);
    fs.formatPen("ASamplers               (%d)\n",              counts.animationSamplers);
    fs.formatPen("Buffers     %011p (%d)\n", buffers,           counts.buffers);
    fs.formatPen("BufferViews %011p (%d)\n", bufferViews,       counts.bufferViews);
    fs.formatPen("Cameras     %011p (%d)\n", cameras,           counts.cameras);
    fs.formatPen("Images      %011p (%d)\n", images,            counts.images);
    fs.formatPen("Materials   %011p (%d)\n", materials,         counts.materials);
    fs.formatPen("Meshs       %011p (%d)\n", meshes,            counts.meshes);
    fs.formatPen("MAttributes %011p (%d)\n", meshAttributes,    counts.meshAttributes);
    fs.formatPen("MPrimitives %011p (%d)\n", meshPrimitives,    counts.meshPrimitives);
    fs.formatPen("MTargets    %011p (%d)\n", meshTargets,       counts.meshTargets);
    fs.formatPen("MWeights    %011p (%d)\n", meshWeights,       counts.meshWeights);
    fs.formatPen("Nodes       %011p (%d)\n", nodes,             counts.nodes);
    fs.formatPen("Samplers    %011p (%d)\n", samplers,          counts.samplers);
    fs.formatPen("Scenes      %011p (%d)\n", scenes,            counts.scenes);
    fs.formatPen("Scene Nodes             (%d)\n",              counts.sceneNodes);
    fs.formatPen("Skins       %011p (%d)\n", skins,             counts.skins);
    fs.formatPen("Textures    %011p (%d)\n", textures,          counts.textures);
    fs.formatPen("raw data    %011p (%zu)\n",rawData,           counts.rawDataLen);

    fs.formatPen("scene       %011p (%zu)\n", scene, scene - scenes);

    fs.formatPen("copyright:  %s\n", copyright);
    fs.formatPen("generator:  %s\n", generator);
    fs.formatPen("version:    %s\n", version);
    fs.formatPen("minVersion: %s\n", minVersion);

    fs.terminatePen();

    // printl("test[[[%s]]]", str);

    return str;
}

void Gobj::Counts::print() const {
    ::print(printToFrameStack());
}

char * Gobj::Counts::printToFrameStack() const {
    assert(mm.frameStack && "Frame stack not initialized.");
    FrameStack & fs = *mm.frameStack;

    char * str = (char *)fs.dataHead();

    fs.formatPen("Gobj Counts\n");
    fs.formatPen("Accessor:     %d\n", accessors);
    fs.formatPen("Animation:    %d\n", animations);
    fs.formatPen("AChannels:    %d\n", animationChannels);
    fs.formatPen("ASamplers:    %d\n", animationSamplers);
    fs.formatPen("Buffer:       %d\n", buffers);
    fs.formatPen("BufferView:   %d\n", bufferViews);
    fs.formatPen("Camera:       %d\n", cameras);
    fs.formatPen("Image:        %d\n", images);
    fs.formatPen("Material:     %d\n", materials);
    fs.formatPen("Mesh:         %d\n", meshes);
    fs.formatPen("MAttribute:   %d\n", meshAttributes);
    fs.formatPen("MPrimitive:   %d\n", meshPrimitives);
    fs.formatPen("MTarget:      %d\n", meshTargets);
    fs.formatPen("Node:         %d\n", nodes);
    fs.formatPen("Sampler:      %d\n", samplers);
    fs.formatPen("Scene:        %d\n", scenes);
    fs.formatPen("Scene Nodes:  %d\n", sceneNodes);
    fs.formatPen("Skin:         %d\n", skins);
    fs.formatPen("Texture:      %d\n", textures);
    fs.formatPen("Total size: %zu\n", totalSize());

    fs.terminatePen();

    return str;
}

#endif // DEBUG || DEV_INTERFACE

