#include "Gobj.h"
#include "FrameStack.h"
#if DEBUG
#include "../dev/print.h"
#include "../MrManager.h"
#endif // DEBUG

Gobj::Gobj(Gobj::Counts const & counts) :
    counts(counts)
{
    byte_t * head = (byte_t *)this;
    head += sizeof(Gobj);

    // has string data, create FrameStack to hold it
    if (counts.allStrLen) {
        strings = new (head) FrameStack{counts.allStrLen};
        head += strings->totalSize();
    }

    if (counts.accessors) {
        accessors = (Accessor *)head;
        for (uint16_t i = 0; i < counts.accessors; ++i) {
            new (head) Accessor{};
            head += sizeof(Accessor);
        }
    }

    if (counts.animations) {
        animations = (Animation *)head;
        for (uint16_t i = 0; i < counts.animations; ++i) {
            new (head) Animation{};
            head += sizeof(Animation);
        }
    }

    if (counts.animationChannels) {
        animationChannels = (AnimationChannel *)head;
        for (uint16_t i = 0; i < counts.animationChannels; ++i) {
            new (head) AnimationChannel{};
            head += sizeof(AnimationChannel);
        }
    }

    if (counts.animationSamplers) {
        animationSamplers = (AnimationSampler *)head;
        for (uint16_t i = 0; i < counts.animationSamplers; ++i) {
            new (head) AnimationSampler{};
            head += sizeof(AnimationSampler);
        }
    }

    if (counts.buffers) {
        buffers = (Buffer *)head;
        for (uint16_t i = 0; i < counts.buffers; ++i) {
            new (head) Buffer{};
            head += sizeof(Buffer);
        }
    }

    if (counts.bufferViews) {
        bufferViews = (BufferView *)head;
        for (uint16_t i = 0; i < counts.bufferViews; ++i) {
            new (head) BufferView{};
            head += sizeof(BufferView);
        }
    }

    if (counts.cameras) {
        cameras = (Camera *)head;
        for (uint16_t i = 0; i < counts.cameras; ++i) {
            new (head) Camera{};
            head += sizeof(Camera);
        }
    }

    if (counts.images) {
        images = (Image *)head;
        for (uint16_t i = 0; i < counts.images; ++i) {
            new (head) Image{};
            head += sizeof(Image);
        }
    }

    if (counts.materials) {
        materials = (Material *)head;
        for (uint16_t i = 0; i < counts.materials; ++i) {
            new (head) Material{};
            head += sizeof(Material);
        }
    }

    if (counts.meshes) {
        meshes = (Mesh *)head;
        for (uint16_t i = 0; i < counts.meshes; ++i) {
            new (head) Mesh{};
            head += sizeof(Mesh);
        }
    }

    if (counts.meshAttributes) {
        meshAttributes = (MeshAttribute *)head;
        for (uint16_t i = 0; i < counts.meshAttributes; ++i) {
            new (head) MeshAttribute{};
            head += sizeof(MeshAttribute);
        }
    }

    if (counts.meshPrimatives) {
        meshPrimatives = (MeshPrimative *)head;
        for (uint16_t i = 0; i < counts.meshPrimatives; ++i) {
            new (head) MeshPrimative{};
            head += sizeof(MeshPrimative);
        }
    }

    if (counts.nodes) {
        nodes = (Node *)head;
        for (uint16_t i = 0; i < counts.nodes; ++i) {
            new (head) Node{};
            head += sizeof(Node);
        }
    }

    if (counts.samplers) {
        samplers = (Sampler *)head;
        for (uint16_t i = 0; i < counts.samplers; ++i) {
            new (head) Sampler{};
            head += sizeof(Sampler);
        }
    }

    if (counts.scenes) {
        scenes = (Scene *)head;
        for (uint16_t i = 0; i < counts.scenes; ++i) {
            new (head) Scene{};
            head += sizeof(Scene);
        }
    }

    if (counts.sceneNodes) {
        sceneNodes = (Node **)head;
        for (uint16_t i = 0; i < counts.sceneNodes; ++i) {
            *(Node **)head = nullptr;
            head += sizeof(Node *);
        }
    }

    if (counts.skins) {
        skins = (Skin *)head;
        for (uint16_t i = 0; i < counts.skins; ++i) {
            new (head) Skin{};
            head += sizeof(Skin);
        }
    }

    if (counts.textures) {
        textures = (Texture *)head;
        for (uint16_t i = 0; i < counts.textures; ++i) {
            new (head) Texture{};
            head += sizeof(Texture);
        }
    }
}

byte_t const * Gobj::data () const { return (byte_t *)this + sizeof(Gobj); }
FrameStack const * Gobj::stringStack () const { return (FrameStack *)data(); }

size_t Gobj::Counts::totalSize() const {
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
        sizeof(Gobj::MeshAttribute)     * meshAttributes +
        sizeof(Gobj::MeshPrimative)     * meshPrimatives +
        sizeof(Gobj::Node)              * nodes +
        sizeof(Gobj::Sampler)           * samplers +
        sizeof(Gobj::Scene)             * scenes +
        sizeof(Gobj::Node *)            * sceneNodes +
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

    fs.formatPen("Accessor    %011p (%d)\n", accessors,         counts.accessors);
    fs.formatPen("Animation   %011p (%d)\n", animations,        counts.animations);
    fs.formatPen("AChannels               (%d)\n",              counts.animationChannels);
    fs.formatPen("ASamplers               (%d)\n",              counts.animationSamplers);
    fs.formatPen("Buffer      %011p (%d)\n", buffers,           counts.buffers);
    fs.formatPen("BufferView  %011p (%d)\n", bufferViews,       counts.bufferViews);
    fs.formatPen("Camera      %011p (%d)\n", cameras,           counts.cameras);
    fs.formatPen("Image       %011p (%d)\n", images,            counts.images);
    fs.formatPen("Material    %011p (%d)\n", materials,         counts.materials);
    fs.formatPen("Mesh        %011p (%d)\n", meshes,            counts.meshes);
    fs.formatPen("MAttribute  %011p (%d)\n", meshAttributes,    counts.meshAttributes);
    fs.formatPen("MPrimative  %011p (%d)\n", meshPrimatives,    counts.meshPrimatives);
    fs.formatPen("Node        %011p (%d)\n", nodes,             counts.nodes);
    fs.formatPen("Sampler     %011p (%d)\n", samplers,          counts.samplers);
    fs.formatPen("Scene       %011p (%d)\n", scenes,            counts.scenes);
    fs.formatPen("Scene Nodes             (%d)\n",              counts.sceneNodes);
    fs.formatPen("Skin        %011p (%d)\n", skins,             counts.skins);
    fs.formatPen("Texture     %011p (%d)\n", textures,          counts.textures);

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
    fs.formatPen("MPrimative:   %d\n", meshPrimatives);
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

#endif // DEBUG
