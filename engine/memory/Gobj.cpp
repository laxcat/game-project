#include "Gobj.h"
#include <new>
#include <glm/gtx/matrix_decompose.hpp>
#include "FrameStack.h"
#include "mem_utils.h"
#include "../common/string_utils.h"
#if DEBUG || DEV_INTERFACE
#include "../dev/print.h"
#include "../MrManager.h"
#endif // DEBUG || DEV_INTERFACE

#define ALIGN_SIZE(SIZE) alignSize(SIZE, Gobj::Align)
#define ALIGN_PTR(PTR)  alignPtr(PTR, Gobj::Align)

Gobj::Gobj(Gobj::Counts const & maxCounts) :
    maxCounts(maxCounts)
{
    // don't align base head, we assume Gobj has been aligned externally (MemMan)
    byte_t * head = (byte_t *)this;
    head += ALIGN_SIZE(sizeof(Gobj));

    // has string data, create FrameStack to hold it
    if (maxCounts.allStrLen) {
        strings = new (head) FrameStack{maxCounts.allStrLen};
        head += ALIGN_SIZE(strings->totalSize());
    }

    if (maxCounts.accessors) {
        accessors = (Accessor *)head;
        for (uint16_t i = 0; i < maxCounts.accessors; ++i) {
            new (head) Accessor{};
            head += sizeof(Accessor);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.animations) {
        animations = (Animation *)head;
        for (uint16_t i = 0; i < maxCounts.animations; ++i) {
            new (head) Animation{};
            head += sizeof(Animation);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.animationChannels) {
        animationChannels = (AnimationChannel *)head;
        for (uint16_t i = 0; i < maxCounts.animationChannels; ++i) {
            new (head) AnimationChannel{};
            head += sizeof(AnimationChannel);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.animationSamplers) {
        animationSamplers = (AnimationSampler *)head;
        for (uint16_t i = 0; i < maxCounts.animationSamplers; ++i) {
            new (head) AnimationSampler{};
            head += sizeof(AnimationSampler);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.buffers) {
        buffers = (Buffer *)head;
        for (uint16_t i = 0; i < maxCounts.buffers; ++i) {
            new (head) Buffer{};
            head += sizeof(Buffer);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.bufferViews) {
        bufferViews = (BufferView *)head;
        for (uint16_t i = 0; i < maxCounts.bufferViews; ++i) {
            new (head) BufferView{};
            head += sizeof(BufferView);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.cameras) {
        cameras = (Camera *)head;
        for (uint16_t i = 0; i < maxCounts.cameras; ++i) {
            new (head) Camera{};
            head += sizeof(Camera);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.images) {
        images = (Image *)head;
        for (uint16_t i = 0; i < maxCounts.images; ++i) {
            new (head) Image{};
            head += sizeof(Image);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.materials) {
        materials = (Material *)head;
        for (uint16_t i = 0; i < maxCounts.materials; ++i) {
            new (head) Material{};
            head += sizeof(Material);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.meshes) {
        meshes = (Mesh *)head;
        for (uint16_t i = 0; i < maxCounts.meshes; ++i) {
            new (head) Mesh{};
            head += sizeof(Mesh);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.meshAttributes) {
        meshAttributes = (MeshAttribute *)head;
        for (uint16_t i = 0; i < maxCounts.meshAttributes; ++i) {
            new (head) MeshAttribute{};
            head += sizeof(MeshAttribute);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.meshPrimitives) {
        meshPrimitives = (MeshPrimitive *)head;
        for (uint16_t i = 0; i < maxCounts.meshPrimitives; ++i) {
            new (head) MeshPrimitive{};
            head += sizeof(MeshPrimitive);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.meshTargets) {
        meshTargets = (MeshTarget *)head;
        for (uint16_t i = 0; i < maxCounts.meshTargets; ++i) {
            new (head) MeshTarget{};
            head += sizeof(MeshTarget);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.meshWeights) {
        meshWeights = (float *)head;
        for (uint16_t i = 0; i < maxCounts.meshWeights; ++i) {
            *(float *)head = 0.f;
            head += sizeof(float);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.nodes) {
        nodes = (Node *)head;
        for (uint16_t i = 0; i < maxCounts.nodes; ++i) {
            new (head) Node{};
            head += sizeof(Node);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.nodeChildren) {
        nodeChildren = (Node **)head;
        for (uint16_t i = 0; i < maxCounts.nodeChildren; ++i) {
            *(Node **)head = nullptr;
            head += sizeof(Node *);
        }
        // aligns on group, not on each ptr
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.nodeWeights) {
        nodeWeights = (float *)head;
        for (uint16_t i = 0; i < maxCounts.nodeWeights; ++i) {
            *(float *)head = 0.f;
            head += sizeof(float);
        }
        // aligns on group, not on each ptr
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.samplers) {
        samplers = (Sampler *)head;
        for (uint16_t i = 0; i < maxCounts.samplers; ++i) {
            new (head) Sampler{};
            head += sizeof(Sampler);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.scenes) {
        scenes = (Scene *)head;
        for (uint16_t i = 0; i < maxCounts.scenes; ++i) {
            new (head) Scene{};
            head += sizeof(Scene);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.skins) {
        skins = (Skin *)head;
        for (uint16_t i = 0; i < maxCounts.skins; ++i) {
            new (head) Skin{};
            head += sizeof(Skin);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.textures) {
        textures = (Texture *)head;
        for (uint16_t i = 0; i < maxCounts.textures; ++i) {
            new (head) Texture{};
            head += sizeof(Texture);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (maxCounts.buffers) {
        rawData = head;
    }
}

void Gobj::setStatus(Status status) {
    _mutex.lock();
    _status = status;
    _mutex.unlock();
}

bool Gobj::isReadyToDraw() const {
    std::lock_guard<std::mutex> guard{_mutex};
    return (_status == STATUS_READY_TO_DRAW);
}

void Gobj::copy(Gobj * src) {
    if (src->strings) {
        memcpy(strings, src->strings, src->strings->totalSize());
    }
    for (uint16_t i = 0; i < src->counts.accessors && i < maxCounts.accessors; ++i) {
        accessors[i].copy(src->accessors + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animations && i < maxCounts.animations; ++i) {
        animations[i].copy(src->animations + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animationChannels && i < maxCounts.animationChannels; ++i) {
        animationChannels[i].copy(src->animationChannels + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animationSamplers && i < maxCounts.animationSamplers; ++i) {
        animationSamplers[i].copy(src->animationSamplers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.buffers && i < maxCounts.buffers; ++i) {
        buffers[i].copy(src->buffers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.bufferViews && i < maxCounts.bufferViews; ++i) {
        bufferViews[i].copy(src->bufferViews + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.cameras && i < maxCounts.cameras; ++i) {
        cameras[i].copy(src->cameras + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.images && i < maxCounts.images; ++i) {
        images[i].copy(src->images + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.materials && i < maxCounts.materials; ++i) {
        materials[i].copy(src->materials + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshes && i < maxCounts.meshes; ++i) {
        meshes[i].copy(src->meshes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshAttributes && i < maxCounts.meshAttributes; ++i) {
        meshAttributes[i].copy(src->meshAttributes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshPrimitives && i < maxCounts.meshPrimitives; ++i) {
        meshPrimitives[i].copy(src->meshPrimitives + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshTargets && i < maxCounts.meshTargets; ++i) {
        meshTargets[i].copy(src->meshTargets + i, this, src);
    }
    memcpy(meshWeights, src->meshWeights, sizeof(float) * min(src->counts.meshWeights, maxCounts.meshWeights));
    for (uint16_t i = 0; i < src->counts.nodes && i < maxCounts.nodes; ++i) {
        nodes[i].copy(src->nodes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.nodeChildren && i < maxCounts.nodeChildren; ++i) {
        nodeChildren[i] = src->nodeRelPtr(src->nodeChildren[i], this);
    }
    memcpy(nodeWeights, src->nodeWeights, sizeof(float) * min(src->counts.nodeWeights, maxCounts.nodeWeights));
    for (uint16_t i = 0; i < src->counts.samplers && i < maxCounts.samplers; ++i) {
        samplers[i].copy(src->samplers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.scenes && i < maxCounts.scenes; ++i) {
        scenes[i].copy(src->scenes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.skins && i < maxCounts.skins; ++i) {
        skins[i].copy(src->skins + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.textures && i < maxCounts.textures; ++i) {
        textures[i].copy(src->textures + i, this, src);
    }
    memcpy(rawData, src->rawData, min(src->counts.rawDataLen, maxCounts.rawDataLen));
    // maxCounts = src->maxCounts; // don't copy the maxCounts! Gobj ctor requires maxCounts and gets set permenantly.
    counts = src->counts;
    scene = src->sceneRelPtr(src->scene, this);
    bounds = src->bounds;
    copyright = src->stringRelPtr(src->copyright, this);
    generator = src->stringRelPtr(src->generator, this);
    version = src->stringRelPtr(src->version, this);
    minVersion = src->stringRelPtr(src->minVersion, this);
    loadedDirName = src->stringRelPtr(src->loadedDirName, this);
    #if DEBUG
    jsonStr = src->stringRelPtr(src->jsonStr, this);
    #endif // DEBUG

    printl("GOBJ DEEP COPY");
    printl("SRC: %p", src);
    src->print();
    printl("DST: %p", this);
    this->print();
}

void Gobj::Accessor::copy(Accessor * accessor, Gobj * dst, Gobj * src) {
    memcpy(this, accessor, sizeof(Accessor));
    bufferView = src->bufferViewRelPtr(accessor->bufferView, dst);
    name = src->stringRelPtr(accessor->name, dst);
    renderHandle = UINT16_MAX;
}

void Gobj::Animation::copy(Animation * animation, Gobj * dst, Gobj * src) {
    channels = src->animationChannelRelPtr(animation->channels, dst);
    nChannels = animation->nChannels;
    samplers = src->animationSamplerRelPtr(animation->samplers, dst);
    nSamplers = animation->nSamplers;
    name = src->stringRelPtr(animation->name, dst);
}

void Gobj::AnimationChannel::copy(AnimationChannel * channel, Gobj * dst, Gobj * src) {
    sampler = dst->animationSamplers + (channel->sampler - src->animationSamplers);
    node = dst->nodes + (channel->node - src->nodes);
    path = channel->path;
}

void Gobj::AnimationSampler::copy(AnimationSampler * sampler, Gobj * dst, Gobj * src) {
    input = dst->accessors + (sampler->input - src->accessors);
    interpolation = sampler->interpolation;
    output = dst->accessors + (sampler->output - src->accessors);
}

void Gobj::Buffer::copy(Buffer * buffer, Gobj * dst, Gobj * src) {
    data = src->rawDataRelPtr(buffer->data, dst);
    byteLength = buffer->byteLength;
    name = src->stringRelPtr(buffer->name, dst);
}

void Gobj::BufferView::copy(BufferView * bufferView, Gobj * dst, Gobj * src) {
    buffer = src->bufferRelPtr(bufferView->buffer, dst);
    byteOffset = bufferView->byteOffset;
    byteLength = bufferView->byteLength;
    byteStride = bufferView->byteStride;
    target = bufferView->target;
    name = src->stringRelPtr(bufferView->name, dst);
}

void Gobj::Camera::copy(Camera * camera, Gobj * dst, Gobj * src) {
    memcpy(_data, camera->_data, sizeof(float) * 4);
    orthographic = (camera->orthographic) ? (CameraOrthographic *)_data : nullptr;
    perspective = (camera->perspective)   ? (CameraPerspective * )_data : nullptr;
    type = camera->type;
    name = src->stringRelPtr(camera->name, dst);
}

void Gobj::Image::copy(Image * image, Gobj * dst, Gobj * src) {
    mimeType = image->mimeType;
    bufferView = src->bufferViewRelPtr(image->bufferView, dst);
    uri  = src->stringRelPtr(image->uri, dst);
    name = src->stringRelPtr(image->name, dst);
}

void Gobj::Material::copy(Material * material, Gobj * dst, Gobj * src) {
    memcpy(this, material, sizeof(Material));
    emissiveTexture = src->textureRelPtr(material->emissiveTexture, dst);
    normalTexture = src->textureRelPtr(material->normalTexture, dst);
    occlusionTexture = src->textureRelPtr(material->occlusionTexture, dst);
    baseColorTexture = src->textureRelPtr(material->baseColorTexture, dst);
    metallicRoughnessTexture = src->textureRelPtr(material->metallicRoughnessTexture, dst);
    name = src->stringRelPtr(material->name, dst);
}

void Gobj::Mesh::copy(Mesh * mesh, Gobj * dst, Gobj * src) {
    primitives = src->meshPrimitiveRelPtr(mesh->primitives, dst);
    nPrimitives = mesh->nPrimitives;
    weights = src->meshWeightRelPtr(mesh->weights, dst);
    nWeights = mesh->nWeights;
    name = src->stringRelPtr(mesh->name, dst);
}

void Gobj::MeshAttribute::copy(MeshAttribute * meshAttribute, Gobj * dst, Gobj * src) {
    type = meshAttribute->type;
    accessor = src->accessorRelPtr(meshAttribute->accessor, dst);
}

void Gobj::MeshPrimitive::copy(MeshPrimitive * meshPrimitive, Gobj * dst, Gobj * src) {
    attributes = src->meshAttributeRelPtr(meshPrimitive->attributes, dst);
    nAttributes = meshPrimitive->nAttributes;
    indices = src->accessorRelPtr(meshPrimitive->indices, dst);
    material = src->materialRelPtr(meshPrimitive->material, dst);
    mode = meshPrimitive->mode;
    targets = src->meshTargetRelPtr(meshPrimitive->targets, dst);
    nTargets = meshPrimitive->nTargets;
}

void Gobj::MeshTarget::copy(MeshTarget * meshTarget, Gobj * dst, Gobj * src) {
    attributes = src->meshAttributeRelPtr(meshTarget->attributes, dst);
    nAttributes = meshTarget->nAttributes;
}

void Gobj::Node::copy(Node * node, Gobj * dst, Gobj * src) {
    camera = src->cameraRelPtr(node->camera, dst);
    children = src->nodeChildrenRelPtr(node->children, dst);
    nChildren = node->nChildren;
    skin = src->skinRelPtr(node->skin, dst);
    matrix = node->matrix;
    mesh = src->meshRelPtr(node->mesh, dst);
    rotation = node->rotation;
    scale = node->scale;
    translation = node->translation;
    weights = src->meshWeightRelPtr(node->weights, dst);
    name = src->stringRelPtr(node->name, dst);
}

void Gobj::Sampler::copy(Sampler * sampler, Gobj * dst, Gobj * src) {
    magFilter = sampler->magFilter;
    minFilter = sampler->minFilter;
    wrapS = sampler->wrapS;
    wrapT = sampler->wrapT;
    name = src->stringRelPtr(sampler->name, dst);
 }

void Gobj::Scene::copy(Scene * scene, Gobj * dst, Gobj * src) {
    nodes = src->nodeChildrenRelPtr(scene->nodes, dst);
    nNodes = scene->nNodes;
    memcpy(name, scene->name, Scene::NameMax);
}

void Gobj::Skin::copy(Skin * skin, Gobj * dst, Gobj * src) {
    inverseBindMatrices = src->accessorRelPtr(skin->inverseBindMatrices, dst);
    skeleton = src->nodeRelPtr(skin->skeleton, dst);
    joints = src->nodeChildrenRelPtr(skin->joints, dst);
    nJoints = skin->nJoints;
    name = src->stringRelPtr(skin->name, dst);
}

void Gobj::Texture::copy(Texture * texture, Gobj * dst, Gobj * src) {
    sampler = src->samplerRelPtr(texture->sampler, dst);
    source = src->imageRelPtr(texture->source, dst);
    name = src->stringRelPtr(texture->name, dst);
}

char const * Gobj::stringRelPtr(char const * str, Gobj * dst) const {
    if (str == nullptr) return nullptr;
    return (char const *)dst->strings->data() + (str - (char const *)strings->data());
}
Gobj::Accessor * Gobj::accessorRelPtr(Accessor * accessor, Gobj * dst) const {
    if (accessor == nullptr) return nullptr;
    return dst->accessors + (accessor - accessors);
}
Gobj::Animation * Gobj::animationRelPtr(Animation * animation, Gobj * dst) const {
    if (animation == nullptr) return nullptr;
    return dst->animations + (animation - animations);
}
Gobj::AnimationChannel * Gobj::animationChannelRelPtr(AnimationChannel * channel, Gobj * dst) const {
    if (channel == nullptr) return nullptr;
    return dst->animationChannels + (channel - animationChannels);
}
Gobj::AnimationSampler * Gobj::animationSamplerRelPtr(AnimationSampler * sampler, Gobj * dst) const {
    if (sampler == nullptr) return nullptr;
    return dst->animationSamplers + (sampler - animationSamplers);
}
Gobj::Buffer * Gobj::bufferRelPtr(Buffer * buffer, Gobj * dst) const {
    if (buffer == nullptr) return nullptr;
    return dst->buffers + (buffer - buffers);
}
Gobj::BufferView * Gobj::bufferViewRelPtr(BufferView * bufferView, Gobj * dst) const {
    if (bufferView == nullptr) return nullptr;
    return dst->bufferViews + (bufferView - bufferViews);
}
Gobj::Camera * Gobj::cameraRelPtr(Camera * camera, Gobj * dst) const {
    if (camera == nullptr) return nullptr;
    return dst->cameras + (camera - cameras);
}
Gobj::Image * Gobj::imageRelPtr(Image * image, Gobj * dst) const {
    if (image == nullptr) return nullptr;
    return dst->images + (image - images);
}
Gobj::Material * Gobj::materialRelPtr(Material * material, Gobj * dst) const {
    if (material == nullptr) return nullptr;
    return dst->materials + (material - materials);
}
Gobj::Mesh * Gobj::meshRelPtr(Mesh * mesh, Gobj * dst) const {
    if (mesh == nullptr) return nullptr;
    return dst->meshes + (mesh - meshes);
}
Gobj::MeshAttribute * Gobj::meshAttributeRelPtr(MeshAttribute * meshAttribute, Gobj * dst) const {
    if (meshAttribute == nullptr) return nullptr;
    return dst->meshAttributes + (meshAttribute - meshAttributes);
}
Gobj::MeshPrimitive * Gobj::meshPrimitiveRelPtr(MeshPrimitive * meshPrimitive, Gobj * dst) const {
    if (meshPrimitive == nullptr) return nullptr;
    return dst->meshPrimitives + (meshPrimitive - meshPrimitives);
}
Gobj::MeshTarget * Gobj::meshTargetRelPtr(MeshTarget * meshTarget, Gobj * dst) const {
    if (meshTarget == nullptr) return nullptr;
    return dst->meshTargets + (meshTarget - meshTargets);
}
float * Gobj::meshWeightRelPtr(float * meshWeight, Gobj * dst) const {
    if (meshWeight == nullptr) return nullptr;
    return dst->meshWeights + (meshWeight - meshWeights);
}
Gobj::Node * Gobj::nodeRelPtr(Node * node, Gobj * dst) const {
    if (node == nullptr) return nullptr;
    return dst->nodes + (node - nodes);
}
Gobj::Node ** Gobj::nodeChildrenRelPtr(Node ** nodeChildren, Gobj * dst) const {
    if (nodeChildren == nullptr) return nullptr;
    return dst->nodeChildren + (nodeChildren - this->nodeChildren);
}
float * Gobj::nodeWeightRelPtr(float * nodeWeight, Gobj * dst) const {
    if (nodeWeight == nullptr) return nullptr;
    return dst->nodeWeights + (nodeWeight - nodeWeights);
}
Gobj::Sampler * Gobj::samplerRelPtr(Sampler * sampler, Gobj * dst) const {
    if (sampler == nullptr) return nullptr;
    return dst->samplers + (sampler - samplers);
}
Gobj::Scene * Gobj::sceneRelPtr(Scene * scene, Gobj * dst) const {
    if (scene == nullptr) return nullptr;
    return dst->scenes + (scene - scenes);
}
Gobj::Skin * Gobj::skinRelPtr(Skin * skin, Gobj * dst) const {
    if (skin == nullptr) return nullptr;
    return dst->skins + (skin - skins);
}
Gobj::Texture * Gobj::textureRelPtr(Texture * texture, Gobj * dst) const {
    if (texture == nullptr) return nullptr;
    return dst->textures + (texture - textures);
}
byte_t * Gobj::rawDataRelPtr(byte_t * data, Gobj * dst) const {
    if (data == nullptr) return nullptr;
    return dst->rawData + (data - rawData);
}

bool Gobj::hasMemoryFor(Counts const & additionalCounts) const {
    if (counts.allStrLen          + additionalCounts.allStrLen         > maxCounts.allStrLen)         return false;
    if (counts.accessors          + additionalCounts.accessors         > maxCounts.accessors)         return false;
    if (counts.animations         + additionalCounts.animations        > maxCounts.animations)        return false;
    if (counts.animationChannels  + additionalCounts.animationChannels > maxCounts.animationChannels) return false;
    if (counts.animationSamplers  + additionalCounts.animationSamplers > maxCounts.animationSamplers) return false;
    if (counts.buffers            + additionalCounts.buffers           > maxCounts.buffers)           return false;
    if (counts.bufferViews        + additionalCounts.bufferViews       > maxCounts.bufferViews)       return false;
    if (counts.cameras            + additionalCounts.cameras           > maxCounts.cameras)           return false;
    if (counts.images             + additionalCounts.images            > maxCounts.images)            return false;
    if (counts.materials          + additionalCounts.materials         > maxCounts.materials)         return false;
    if (counts.meshes             + additionalCounts.meshes            > maxCounts.meshes)            return false;
    if (counts.meshAttributes     + additionalCounts.meshAttributes    > maxCounts.meshAttributes)    return false;
    if (counts.meshPrimitives     + additionalCounts.meshPrimitives    > maxCounts.meshPrimitives)    return false;
    if (counts.meshTargets        + additionalCounts.meshTargets       > maxCounts.meshTargets)       return false;
    if (counts.meshWeights        + additionalCounts.meshWeights       > maxCounts.meshWeights)       return false;
    if (counts.nodes              + additionalCounts.nodes             > maxCounts.nodes)             return false;
    if (counts.nodeChildren       + additionalCounts.nodeChildren      > maxCounts.nodeChildren)      return false;
    if (counts.nodeWeights        + additionalCounts.nodeWeights       > maxCounts.nodeWeights)       return false;
    if (counts.samplers           + additionalCounts.samplers          > maxCounts.samplers)          return false;
    if (counts.scenes             + additionalCounts.scenes            > maxCounts.scenes)            return false;
    if (counts.skins              + additionalCounts.skins             > maxCounts.skins)             return false;
    if (counts.textures           + additionalCounts.textures          > maxCounts.textures)          return false;
    if (counts.rawDataLen         + additionalCounts.rawDataLen        > maxCounts.rawDataLen)        return false;

    return true;
}

void Gobj::traverse(TraverseFns const & fns, glm::mat4 const & parentTransform) {
    // if scene is set
    if (scene) {
        // each root node of scene
        for (uint16_t nodeIndex = 0; nodeIndex < scene->nNodes; ++nodeIndex) {
            traverseNode(scene->nodes[nodeIndex], fns, parentTransform);
        }
    }
    // otherwise calculate all meshes
    else {
        // each mesh
        for (uint16_t meshIndex = 0; meshIndex < counts.meshes; ++meshIndex) {
            (meshes + meshIndex)->traverse(fns, parentTransform);
        }
    }
}

void Gobj::traverseNode(Node * node, TraverseFns const & fns, glm::mat4 const & parentTransform) {
    // calc global transform
    glm::mat4 global = parentTransform * node->matrix;
    // run node fn if present
    if (fns.eachNode) {
        fns.eachNode(node, global);
    }
    // traverse mesh
    if (node->mesh) {
        node->mesh->traverse(fns, global);
    }
    // traverse child nodes
    for (uint16_t nodeIndex = 0; nodeIndex < node->nChildren; ++nodeIndex) {
        traverseNode(node->children[nodeIndex], fns, global);
    }
}

void Gobj::updateBoundsForCurrentScene() {
    traverse(
        {.eachPosAccr = [this](Gobj::Accessor * accessor, glm::mat4 const & global) {
            glm::vec4 min = global * glm::vec4{*(glm::vec3 *)accessor->min, 1.f};
            glm::vec4 max = global * glm::vec4{*(glm::vec3 *)accessor->max, 1.f};
            // check both min and max as global scale might have flipped things
            // x min
            if (bounds.min.x > min.x) bounds.min.x = min.x;
            if (bounds.min.x > max.x) bounds.min.x = max.x;
            // x max
            if (bounds.max.x < max.x) bounds.max.x = max.x;
            if (bounds.max.x < min.x) bounds.max.x = min.x;
            // y min
            if (bounds.min.y > min.y) bounds.min.y = min.y;
            if (bounds.min.y > max.y) bounds.min.y = max.y;
            // y max
            if (bounds.max.y < max.y) bounds.max.y = max.y;
            if (bounds.max.y < min.y) bounds.max.y = min.y;
            // z min
            if (bounds.min.z > min.z) bounds.min.z = min.z;
            if (bounds.min.z > max.z) bounds.min.z = max.z;
            // z max
            if (bounds.max.z < max.z) bounds.max.z = max.z;
            if (bounds.max.z < min.z) bounds.max.z = min.z;
        }}
    );
}

Gobj::Scene * Gobj::addScene(char const * name, bool makeDefault) {
    if (counts.scenes >= maxCounts.scenes) {
        fprintf(stderr, "Could not create scene.\n");
        return nullptr;
    }
    Scene * s = scenes + counts.scenes;
    ++counts.scenes;
    s->nNodes = 1;
    s->nodes = addNodeChildren(1);
    snprintf(s->name, Scene::NameMax, "%s", name);
    if (makeDefault) {
        scene = s;
    }
    return s;
}

Gobj::Node ** Gobj::addNodeChildren(uint16_t nNodes) {
    if (counts.nodeChildren + nNodes > maxCounts.nodeChildren) {
        fprintf(stderr, "Not enough room to create %u node children.\n", nNodes);
        return nullptr;
    }
    if (counts.nodes + nNodes > maxCounts.nodes) {
        fprintf(stderr, "Not enough room to create %u nodes.\n", nNodes);
        return nullptr;
    }
    // mark the start of the nodeChildren array
    Node ** ret = nodeChildren + counts.nodeChildren;
    // populate array with nodes, incrementing count of used nodes
    for (uint16_t i = 0; i < nNodes; ++i, ++counts.nodes) {
        ret[i] = nodes + counts.nodes;
    }
    // increment count of used nodeChildren
    counts.nodeChildren += nNodes;
    return ret;
}

Gobj::Node * Gobj::addNode(char const * name) {
    if (counts.nodes >= maxCounts.nodes) {
        fprintf(stderr, "Not enough room to create node.\n");
        return nullptr;
    }
    Node * n = nodes + counts.nodes;
    ++counts.nodes;
    n->name = copyStr(name);
    return n;
}

Gobj::Mesh * Gobj::addMesh(char const * name) {
    if (counts.meshes >= maxCounts.meshes) {
        fprintf(stderr, "Could not create mesh.\n");
        return nullptr;
    }
    Mesh * m = meshes + counts.meshes;
    ++counts.meshes;
    m->name = copyStr(name);
    return m;
}

Gobj::MeshPrimitive * Gobj::_addMeshPrimitive(int count, ...) {
    if (counts.meshPrimitives >= maxCounts.meshPrimitives) {
        fprintf(stderr, "Could not create mesh primitive.\n");
        return nullptr;
    }
    if (counts.meshAttributes + count > maxCounts.meshAttributes) {
        fprintf(stderr, "Could not create %d mesh attributes.\n", count);
        return nullptr;
    }
    MeshPrimitive * prim = meshPrimitives + counts.meshPrimitives;
    ++counts.meshPrimitives;
    // prim->nAttributes = count;
    // prim->attributes = meshAttributes + counts.meshAttributes;
    // va_list args;
    // va_start(args, count);
    // for (int i = 0; i < count; ++i) {
    //     Attr a = (Attr)va_arg(args, int);
    //     addMeshAttribute(a);
    // }
    // va_end(args);
    return prim;
}

Gobj::MeshAttribute * Gobj::addMeshAttribute(Attr attr) {
    if (counts.meshAttributes >= maxCounts.meshAttributes) {
        fprintf(stderr, "Could not create mesh attribute %d (%s).\n", (int)attr, attrStr(attr));
        return nullptr;
    }
    MeshAttribute * ma = meshAttributes + counts.meshAttributes;
    ++counts.meshAttributes;
    ma->type = attr;
    return ma;
}

Gobj::Buffer * Gobj::addBuffer(size_t size) {
    if (counts.buffers >= maxCounts.buffers) {
        fprintf(stderr, "Could not create buffer.\n");
        return nullptr;
    }
    if (counts.rawDataLen + size > maxCounts.rawDataLen) {
        fprintf(stderr, "Not enough raw-data space to create buffer of size %zu.\n", size);
        return nullptr;
    }
    Buffer * b = buffers + counts.buffers;
    ++counts.buffers;
    b->byteLength = size;
    b->data = rawData + counts.rawDataLen;
    counts.rawDataLen += size;
    return b;
}

Gobj::BufferView * Gobj::addBufferView() {
    if (counts.bufferViews >= maxCounts.bufferViews) {
        fprintf(stderr, "Could not create buffer view.\n");
        return nullptr;
    }
    BufferView * bv = bufferViews + counts.bufferViews;
    ++counts.bufferViews;
    return bv;
}

Gobj::Accessor * Gobj::addAccessor(char const * name) {
    if (counts.accessors >= maxCounts.accessors) {
        fprintf(stderr, "Could not create accessor.\n");
        return nullptr;
    }
    Accessor * accr = accessors + counts.accessors;
    ++counts.accessors;
    accr->name = copyStr(name);
    return accr;
}

Gobj::Material * Gobj::addMaterial(char const * name) {
    if (counts.materials >= maxCounts.materials) {
        fprintf(stderr, "Could not create material.\n");
        return nullptr;
    }
    Material * m = materials + counts.materials;
    ++counts.materials;
    m->name = copyStr(name);
    return m;
}

Gobj::Mesh * Gobj::makeMesh(MakeMesh const & setup, Buffer ** outBuffer) {
    size_t vertSize = 0;
    for (int i = 0; i < setup.nAttributes; ++i) {
        switch (setup.attributes[i]) {
        case ATTR_POSITION: { vertSize += sizeof(float) * 3; break; }
        case ATTR_NORMAL:   { vertSize += sizeof(float) * 3; break; }
        case ATTR_COLOR0:
        case ATTR_COLOR1:
        case ATTR_COLOR2:
        case ATTR_COLOR3:   { vertSize += sizeof(float) * 4; break; }
        default: {
            fprintf(stderr, "Unexpected vertex attribute. TODO: support more.");
            return nullptr;
        }
        }
    }
    size_t bufferSize =
        vertSize * setup.nVertices +
        sizeof(uint16_t) * setup.nIndices;

    Gobj::Buffer * buffer = nullptr;
    if (bufferSize) {
        buffer = addBuffer(bufferSize);
    }

    // vertex buffer view
    Gobj::BufferView * vertBV = nullptr;
    if (vertSize) {
        vertBV = addBufferView();
        vertBV->buffer = buffer;
        vertBV->byteOffset = 0;
        vertBV->byteLength = vertSize * setup.nVertices;
        vertBV->byteStride = vertSize;
    }

    // vertex accessors
    uint32_t byteOffset = 0;
    Gobj::MeshAttribute * firstMeshAttr = nullptr;
    for (int i = 0; i < setup.nAttributes; ++i) {
        Attr attr = setup.attributes[i];
        Gobj::Accessor * accr = addAccessor(attrStr4(attr));
        accr->byteOffset = byteOffset;
        accr->bufferView = vertBV;
        accr->count = setup.nVertices;
        Gobj::MeshAttribute * meshAttr = addMeshAttribute(attr);
        meshAttr->accessor = accr;

        switch(attr) {
        case ATTR_POSITION:
        case ATTR_NORMAL: {
            accr->componentType = Gobj::Accessor::COMPTYPE_FLOAT;
            accr->type = Gobj::Accessor::TYPE_VEC3;
            break; }
        case ATTR_COLOR0:
        case ATTR_COLOR1:
        case ATTR_COLOR2:
        case ATTR_COLOR3: {
            accr->componentType = Gobj::Accessor::COMPTYPE_FLOAT;
            accr->type = Gobj::Accessor::TYPE_VEC4;
            break; }
        default: {}
        }

        byteOffset += accr->byteSize();
        if (!firstMeshAttr) firstMeshAttr = meshAttr;
    }

    // index accessor and buffer view
    Gobj::Accessor * indexAccr = nullptr;
    if (setup.nIndices) {
        indexAccr = addAccessor(attrStr4(ATTR_INDICES));
        indexAccr->byteOffset = 0;
        indexAccr->componentType = Gobj::Accessor::COMPTYPE_UNSIGNED_SHORT;
        indexAccr->type = Gobj::Accessor::TYPE_SCALAR;
        indexAccr->count = setup.nIndices;
        indexAccr->bufferView = addBufferView();
        indexAccr->bufferView->buffer = buffer;
        indexAccr->bufferView->byteOffset = (vertBV) ? vertBV->byteLength : 0;
        indexAccr->bufferView->byteLength = sizeof(uint16_t) * setup.nIndices;
        indexAccr->bufferView->byteStride = sizeof(uint16_t);
    }

    // mesh / mesh primitive
    Gobj::Mesh * mesh = addMesh(setup.name);
    assert(mesh);
    Gobj::MeshPrimitive * prim = addMeshPrimitive();
    assert(prim);
    mesh->nPrimitives = 1;
    mesh->primitives = prim;
    prim->nAttributes = setup.nAttributes;
    prim->attributes = firstMeshAttr;
    prim->indices = indexAccr;

    if (outBuffer) {
        *outBuffer = buffer;
    }

    return mesh;
}

char * Gobj::formatStr(char const * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char * str = strings->vformatStr(fmt, args);
    va_end(args);
    counts.allStrLen = strings->head();
    return str;
}
char * Gobj::formatPen(char const * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char * str = strings->vformatPen(fmt, args);
    va_end(args);
    counts.allStrLen = strings->head();
    return str;
}
void Gobj::terminatePen() {
    strings->terminatePen();
    counts.allStrLen = strings->head();
}
char * Gobj::copyStr(char const * str, size_t length) {
    char * ret = strings->formatStr("%.*s", length, str);
    counts.allStrLen = strings->head();
    return ret;
}
char * Gobj::copyStr(char const * str) {
    char * ret = strings->formatStr("%s", str);
    counts.allStrLen = strings->head();
    return ret;
}

size_t Gobj::Counts::totalSize() const {
    return
        ALIGN_SIZE(sizeof(Gobj)) +
        ALIGN_SIZE(sizeof(FrameStack) + allStrLen) +
        ALIGN_SIZE(sizeof(Gobj::Accessor)          * accessors) +
        ALIGN_SIZE(sizeof(Gobj::Animation)         * animations) +
        ALIGN_SIZE(sizeof(Gobj::AnimationChannel)  * animationChannels) +
        ALIGN_SIZE(sizeof(Gobj::AnimationSampler)  * animationSamplers) +
        ALIGN_SIZE(sizeof(Gobj::Buffer)            * buffers) +
        ALIGN_SIZE(sizeof(Gobj::BufferView)        * bufferViews) +
        ALIGN_SIZE(sizeof(Gobj::Camera)            * cameras) +
        ALIGN_SIZE(sizeof(Gobj::Image)             * images) +
        ALIGN_SIZE(sizeof(Gobj::Material)          * materials) +
        ALIGN_SIZE(sizeof(Gobj::Mesh)              * meshes) +
        ALIGN_SIZE(sizeof(Gobj::MeshAttribute)     * meshAttributes) +
        ALIGN_SIZE(sizeof(Gobj::MeshPrimitive)     * meshPrimitives) +
        ALIGN_SIZE(sizeof(Gobj::MeshTarget)        * meshTargets) +
        ALIGN_SIZE(sizeof(float)                   * meshWeights) +
        ALIGN_SIZE(sizeof(Gobj::Node)              * nodes) +
        ALIGN_SIZE(sizeof(Gobj::Node *)            * nodeChildren) +
        ALIGN_SIZE(sizeof(float)                   * nodeWeights) +
        ALIGN_SIZE(sizeof(Gobj::Sampler)           * samplers) +
        ALIGN_SIZE(sizeof(Gobj::Scene)             * scenes) +
        ALIGN_SIZE(sizeof(Gobj::Skin)              * skins) +
        ALIGN_SIZE(sizeof(Gobj::Texture)           * textures) +
        ALIGN_SIZE(rawDataLen)
    ;
}

Gobj::Counts Gobj::Counts::operator+(Counts const & other) const {
    Counts ret = *this;
    ret += other;
    return ret;
}

Gobj::Counts & Gobj::Counts::operator+=(Counts const & other) {
    allStrLen          += other.allStrLen;
    accessors          += other.accessors;
    animations         += other.animations;
    animationChannels  += other.animationChannels;
    animationSamplers  += other.animationSamplers;
    buffers            += other.buffers;
    bufferViews        += other.bufferViews;
    cameras            += other.cameras;
    images             += other.images;
    materials          += other.materials;
    meshes             += other.meshes;
    meshAttributes     += other.meshAttributes;
    meshPrimitives     += other.meshPrimitives;
    meshTargets        += other.meshTargets;
    meshWeights        += other.meshWeights;
    nodes              += other.nodes;
    nodeChildren       += other.nodeChildren;
    nodeWeights        += other.nodeWeights;
    samplers           += other.samplers;
    scenes             += other.scenes;
    skins              += other.skins;
    textures           += other.textures;
    rawDataLen         += other.rawDataLen;

    return *this;
}

uint8_t Gobj::Accessor::componentCount() const {
    switch (type) {
    case TYPE_UNDEFINED : return  0;
    case TYPE_SCALAR    : return  1;
    case TYPE_VEC2      : return  2;
    case TYPE_VEC3      : return  3;
    case TYPE_VEC4      : return  4;
    case TYPE_MAT2      : return  4;
    case TYPE_MAT3      : return  9;
    case TYPE_MAT4      : return 16;
    }
}
uint8_t Gobj::Accessor::componentByteSize() const {
    switch (componentType) {
    case COMPTYPE_BYTE           :
    case COMPTYPE_UNSIGNED_BYTE  : return 1;
    case COMPTYPE_SHORT          :
    case COMPTYPE_UNSIGNED_SHORT : return 2;
    case COMPTYPE_UNSIGNED_INT   :
    case COMPTYPE_FLOAT          : return 4;
    }
}
uint32_t Gobj::Accessor::byteSize() const {
    return componentCount() * componentByteSize();
}
float Gobj::Accessor::componentValue(uint32_t index, uint8_t componentIndex) const {
    auto data =
        bufferView->buffer->data +
        byteOffset +
        bufferView->byteOffset +
        bufferView->byteStride * index +
        componentByteSize() * componentIndex;
    switch (componentType) {
    case COMPTYPE_FLOAT:            return *(float *)data;
    case COMPTYPE_UNSIGNED_SHORT:   return (float)*(uint16_t *)data;
    default:                        assert(false && "Unexpected component type.");
    }
}

void Gobj::Accessor::updateMinMax() {
    uint8_t cc = componentCount();
    for (uint32_t i = 0; i < count; ++i) {
        for (uint8_t c = 0; c < cc; ++c) {
            float fval = componentValue(i, c);
            if (min[c] > fval) min[c] = fval;
            if (max[c] < fval) max[c] = fval;
            // printl("i: %u, c: %u, %f", i, c, fval);
        }
    }
}

#if DEBUG || DEV_INTERFACE
void Gobj::Accessor::print(int indent) const { ::print(printToFrameStack(indent)); }
char * Gobj::Accessor::printToFrameStack(int indent) const {
    assert(mm.frameStack && "Frame stack not initialized.");
    assert(bufferView->byteStride && "byteStride must be set.");

    auto data = bufferView->buffer->data + byteOffset + bufferView->byteOffset;
    // auto dataEnd = bufferView->buffer->data + bufferView->byteLength;
    // assert(componentType == COMPTYPE_FLOAT);

    FrameStack & fs = *mm.frameStack;
    char * str = (char *)fs.dataHead();

    fs.formatPen("%*sAccessor: %s (%p)\n", indent,"", name, this);
    fs.formatPen("%*s    count: %u\n", indent,"", count);
    fs.formatPen("%*s    byteOffset: %u\n", indent,"", byteOffset);
    fs.formatPen("%*s    buffer: %p-%p\n", indent,"", bufferView->buffer->data, bufferView->buffer->data + bufferView->buffer->byteLength);
    fs.formatPen("%*s    buffer byteLength: %u\n", indent,"", bufferView->buffer->byteLength);
    fs.formatPen("%*s    bufferView byteLength: %u\n", indent,"", bufferView->byteLength);
    fs.formatPen("%*s    bufferView byteOffset: %u\n", indent,"", bufferView->byteOffset);
    fs.formatPen("%*s    data:\n", indent,"");

    uint8_t cc = componentCount();
    // each element
    for (uint32_t i = 0; i < count; ++i) {
        fs.formatPen("%*s%03u: ", indent+4,"", i);
        // each component
        for (uint8_t c = 0; c < cc; ++c) {
            switch (componentType) {
            case COMPTYPE_FLOAT: {
                fs.formatPen("%+f ", ((float *)data)[c]);
                break; }
            case COMPTYPE_UNSIGNED_SHORT: {
                fs.formatPen("%u ", ((uint16_t *)data)[c]);
                break; }
            default: {}
            }
        }
        fs.formatPen("   (%p)\n", data);
        data += bufferView->byteStride;
    }

    fs.terminatePen();
    return str;
}
#endif // DEBUG || DEV_INTERFACE

void Gobj::Mesh::updateAccessorMinMax() {
    traverse({
        .eachPrim = [this](Gobj::MeshPrimitive * prim, glm::mat4 const &) {
            prim->indices->updateMinMax();
        },
        .eachAccr = [this](Gobj::Accessor * accr, glm::mat4 const &) {
            accr->updateMinMax();
        },
    });
}

void Gobj::Mesh::traverse(TraverseFns const & fns, glm::mat4 const & parentTransform) {
    // run mesh fn if present
    if (fns.eachMesh) {
        fns.eachMesh(this, parentTransform);
    }
    for (int primIndex = 0; primIndex < nPrimitives; ++primIndex) {
        Gobj::MeshPrimitive * prim = primitives + primIndex;
        if (fns.eachPrim) {
            fns.eachPrim(prim, parentTransform);
        }
        if (fns.eachAccr || fns.eachPosAccr) {
            for (int attrIndex = 0; attrIndex < prim->nAttributes; ++attrIndex) {
                Gobj::MeshAttribute * attr = prim->attributes + attrIndex;
                if (fns.eachAccr) {
                    fns.eachAccr(attr->accessor, parentTransform);
                }
                if (attr->type == Gobj::ATTR_POSITION &&  fns.eachPosAccr) {
                    fns.eachPosAccr(attr->accessor, parentTransform);
                }
            }
        }
    }
}

#if DEBUG
void Gobj::MeshAttribute::print(int indent) const { ::print(printToFrameStack(indent)); }
char * Gobj::MeshAttribute::printToFrameStack(int indent) const {
    assert(mm.frameStack && "Frame stack not initialized.");

    FrameStack & fs = *mm.frameStack;
    char * str = (char *)fs.dataHead();

    fs.formatPen("%*sMeshAttribute: (%p)\n", indent,"", this);
    fs.formatPen("%*s    type: %s\n", indent,"", attrStr(type));
    accessor->printToFrameStack(4);
    fs.terminatePen();

    return str;
}
#endif // DEBUG


void Gobj::Node::syncMatrixTRS(bool syncChildren) {
    // is identity?
    if (matrix == glm::mat4{1.f}) {
        // TRS -> matrix
        setTRSToMatrix(syncChildren);
    }
    else {
        // matrix -> TRS
        setMatrixToTRS(syncChildren);
    }
}

void Gobj::Node::setMatrixToTRS(bool syncChildren) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);

    // process children too?
    if (syncChildren) {
        for (uint16_t i = 0; i < nChildren; ++i) {
            children[i]->setMatrixToTRS(syncChildren);
        }
    }
}

void Gobj::Node::setTRSToMatrix(bool syncChildren) {
    matrix = glm::mat4{1.f};
    matrix = glm::translate(matrix, translation);
    matrix *= glm::mat4_cast(rotation);
    matrix = glm::scale(matrix, scale);

    // process children too?
    if (syncChildren) {
        for (uint16_t i = 0; i < nChildren; ++i) {
            children[i]->setTRSToMatrix(syncChildren);
        }
    }
}

Gobj::Accessor::Type Gobj::accessorTypeFromStr(char const * str) {
    if (strEqu(str, "SCALAR")) return Accessor::TYPE_SCALAR;
    if (strEqu(str, "VEC2"  )) return Accessor::TYPE_VEC2;
    if (strEqu(str, "VEC3"  )) return Accessor::TYPE_VEC3;
    if (strEqu(str, "VEC4"  )) return Accessor::TYPE_VEC4;
    if (strEqu(str, "MAT2"  )) return Accessor::TYPE_MAT2;
    if (strEqu(str, "MAT3"  )) return Accessor::TYPE_MAT3;
    if (strEqu(str, "MAT4"  )) return Accessor::TYPE_MAT4;
    return Accessor::TYPE_UNDEFINED;
}

Gobj::AnimationChannel::Target Gobj::animationTargetFromStr(char const * str) {
    if (strEqu(str, "weights"    )) return AnimationChannel::TARGET_WEIGHTS;
    if (strEqu(str, "translation")) return AnimationChannel::TARGET_TRANSLATION;
    if (strEqu(str, "rotation"   )) return AnimationChannel::TARGET_ROTATION;
    if (strEqu(str, "scale"      )) return AnimationChannel::TARGET_SCALE;
    return AnimationChannel::TARGET_UNDEFINED;
}

Gobj::AnimationSampler::Interpolation Gobj::interpolationFromStr(char const * str) {
    if (strEqu(str, "STEP"       )) return AnimationSampler::INTERP_STEP;
    if (strEqu(str, "CUBICSPLINE")) return AnimationSampler::INTERP_CUBICSPLINE;
    return AnimationSampler::INTERP_LINEAR;
}

Gobj::Camera::Type Gobj::cameraTypeFromStr(char const * str) {
    if (strEqu(str, "orthographic")) return Camera::TYPE_ORTHO;
    if (strEqu(str, "perspective" )) return Camera::TYPE_PERSP;
    return Camera::TYPE_PERSP;
}

Gobj::Image::MIMEType Gobj::imageMIMETypeFromStr(char const * str) {
    if (strEqu(str, "image/jpeg"))  return Image::TYPE_JPEG;
    if (strEqu(str, "image/png" ))  return Image::TYPE_PNG;
    return Image::TYPE_PNG;
}

Gobj::Material::AlphaMode Gobj::alphaModeFromStr(char const * str) {
    if (strEqu(str, "OPAQUE"))    return Material::ALPHA_OPAQUE;
    if (strEqu(str, "MASK" ))     return Material::ALPHA_MASK;
    if (strEqu(str, "BLEND" ))    return Material::ALPHA_BLEND;
    return Material::ALPHA_OPAQUE;
}

Gobj::Attr Gobj::attrFromStr(char const * str) {
    if (strEqu(str, "POSITION"))   return ATTR_POSITION;
    if (strEqu(str, "NORMAL"))     return ATTR_NORMAL;
    if (strEqu(str, "TANGENT"))    return ATTR_TANGENT;
    if (strEqu(str, "BITANGENT"))  return ATTR_BITANGENT;
    if (strEqu(str, "COLOR0"))     return ATTR_COLOR0;
    if (strEqu(str, "COLOR1"))     return ATTR_COLOR1;
    if (strEqu(str, "COLOR2"))     return ATTR_COLOR2;
    if (strEqu(str, "COLOR3"))     return ATTR_COLOR3;
    if (strEqu(str, "INDICES"))    return ATTR_INDICES;
    if (strEqu(str, "WEIGHT"))     return ATTR_WEIGHT;
    if (strEqu(str, "TEXCOORD_0")) return ATTR_TEXCOORD0;
    if (strEqu(str, "TEXCOORD_1")) return ATTR_TEXCOORD1;
    if (strEqu(str, "TEXCOORD_2")) return ATTR_TEXCOORD2;
    if (strEqu(str, "TEXCOORD_3")) return ATTR_TEXCOORD3;
    if (strEqu(str, "TEXCOORD_4")) return ATTR_TEXCOORD4;
    if (strEqu(str, "TEXCOORD_5")) return ATTR_TEXCOORD5;
    if (strEqu(str, "TEXCOORD_6")) return ATTR_TEXCOORD6;
    if (strEqu(str, "TEXCOORD_7")) return ATTR_TEXCOORD7;
    if (strEqu(str, "TEXCOORD0"))  return ATTR_TEXCOORD0;
    if (strEqu(str, "TEXCOORD1"))  return ATTR_TEXCOORD1;
    if (strEqu(str, "TEXCOORD2"))  return ATTR_TEXCOORD2;
    if (strEqu(str, "TEXCOORD3"))  return ATTR_TEXCOORD3;
    if (strEqu(str, "TEXCOORD4"))  return ATTR_TEXCOORD4;
    if (strEqu(str, "TEXCOORD5"))  return ATTR_TEXCOORD5;
    if (strEqu(str, "TEXCOORD6"))  return ATTR_TEXCOORD6;
    if (strEqu(str, "TEXCOORD7"))  return ATTR_TEXCOORD7;
    return ATTR_POSITION;
}

char const * Gobj::attrStr(Attr attr) {
    switch(attr) {
    case ATTR_POSITION:     return "POSITION";
    case ATTR_NORMAL:       return "NORMAL";
    case ATTR_TANGENT:      return "TANGENT";
    case ATTR_BITANGENT:    return "BITANGENT";
    case ATTR_COLOR0:       return "COLOR0";
    case ATTR_COLOR1:       return "COLOR1";
    case ATTR_COLOR2:       return "COLOR2";
    case ATTR_COLOR3:       return "COLOR3";
    case ATTR_INDICES:      return "INDICES";
    case ATTR_WEIGHT:       return "WEIGHT";
    case ATTR_TEXCOORD0:    return "TEXCOORD0";
    case ATTR_TEXCOORD1:    return "TEXCOORD1";
    case ATTR_TEXCOORD2:    return "TEXCOORD2";
    case ATTR_TEXCOORD3:    return "TEXCOORD3";
    case ATTR_TEXCOORD4:    return "TEXCOORD4";
    case ATTR_TEXCOORD5:    return "TEXCOORD5";
    case ATTR_TEXCOORD6:    return "TEXCOORD6";
    case ATTR_TEXCOORD7:    return "TEXCOORD7";
    default:                return "UNKNOWN";
    }
}

char const * Gobj::attrStr4(Attr attr) {
    switch(attr) {
    case ATTR_POSITION:     return "POSI";
    case ATTR_NORMAL:       return "NORM";
    case ATTR_TANGENT:      return "TANG";
    case ATTR_BITANGENT:    return "BTNG";
    case ATTR_COLOR0:       return "COL0";
    case ATTR_COLOR1:       return "COL1";
    case ATTR_COLOR2:       return "COL2";
    case ATTR_COLOR3:       return "COL3";
    case ATTR_INDICES:      return "INDX";
    case ATTR_WEIGHT:       return "WGHT";
    case ATTR_TEXCOORD0:    return "TXC0";
    case ATTR_TEXCOORD1:    return "TXC1";
    case ATTR_TEXCOORD2:    return "TXC2";
    case ATTR_TEXCOORD3:    return "TXC3";
    case ATTR_TEXCOORD4:    return "TXC4";
    case ATTR_TEXCOORD5:    return "TXC5";
    case ATTR_TEXCOORD6:    return "TXC6";
    case ATTR_TEXCOORD7:    return "TXC7";
    default:                return "UNKN";
    }
}

char const * Gobj::accessorTypeStr(Accessor::Type type) {
    switch(type) {
    case Accessor::TYPE_SCALAR: return "SCALAR";
    case Accessor::TYPE_VEC2:   return "VEC2";
    case Accessor::TYPE_VEC3:   return "VEC3";
    case Accessor::TYPE_VEC4:   return "VEC4";
    case Accessor::TYPE_MAT2:   return "MAT2";
    case Accessor::TYPE_MAT3:   return "MAT3";
    case Accessor::TYPE_MAT4:   return "MAT4";
    default:                    return "UNKNOWN";
    }
}

char const * Gobj::accessorComponentTypeStr(Accessor::ComponentType componentType) {
    switch (componentType) {
    case Accessor::COMPTYPE_BYTE:           return "BYTE";
    case Accessor::COMPTYPE_UNSIGNED_BYTE:  return "UBYTE";
    case Accessor::COMPTYPE_SHORT:          return "SHORT";
    case Accessor::COMPTYPE_UNSIGNED_SHORT: return "USHORT";
    case Accessor::COMPTYPE_UNSIGNED_INT:   return "UINT";
    case Accessor::COMPTYPE_FLOAT:          return "FLOAT";
    default:                                return "UNKNOWN";
    }
}


Gobj::Attr Gobj::texCoordAttr(int index) {
    return (Attr)((int)ATTR_TEXCOORD0 + index);
}

bool Gobj::isValid(uint16_t handle) {
    return handle != UINT16_MAX;
}

#if DEBUG || DEV_INTERFACE

void Gobj::print() const {
    ::print(printToFrameStack());
}

char * Gobj::printToFrameStack() const {
    assert(mm.frameStack && "Frame stack not initialized.");
    FrameStack & fs = *mm.frameStack;

    char * str = (char *)fs.dataHead();

    fs.formatPen("strings     %011p (%u/%u)\n", strings,           counts.allStrLen,            maxCounts.allStrLen);
    fs.formatPen("Accessors   %011p (%u/%u)\n", accessors,         counts.accessors,            maxCounts.accessors);
    fs.formatPen("Animation   %011p (%u/%u)\n", animations,        counts.animations,           maxCounts.animations);
    fs.formatPen("AChannels   %011p (%u/%u)\n", animationChannels, counts.animationChannels,    maxCounts.animationChannels);
    fs.formatPen("ASamplers   %011p (%u/%u)\n", animationSamplers, counts.animationSamplers,    maxCounts.animationSamplers);
    fs.formatPen("Buffers     %011p (%u/%u)\n", buffers,           counts.buffers,              maxCounts.buffers);
    fs.formatPen("BufferViews %011p (%u/%u)\n", bufferViews,       counts.bufferViews,          maxCounts.bufferViews);
    fs.formatPen("Cameras     %011p (%u/%u)\n", cameras,           counts.cameras,              maxCounts.cameras);
    fs.formatPen("Images      %011p (%u/%u)\n", images,            counts.images,               maxCounts.images);
    fs.formatPen("Materials   %011p (%u/%u)\n", materials,         counts.materials,            maxCounts.materials);
    fs.formatPen("Meshs       %011p (%u/%u)\n", meshes,            counts.meshes,               maxCounts.meshes);
    fs.formatPen("MAttributes %011p (%u/%u)\n", meshAttributes,    counts.meshAttributes,       maxCounts.meshAttributes);
    fs.formatPen("MPrimitives %011p (%u/%u)\n", meshPrimitives,    counts.meshPrimitives,       maxCounts.meshPrimitives);
    fs.formatPen("MTargets    %011p (%u/%u)\n", meshTargets,       counts.meshTargets,          maxCounts.meshTargets);
    fs.formatPen("MWeights    %011p (%u/%u)\n", meshWeights,       counts.meshWeights,          maxCounts.meshWeights);
    fs.formatPen("Nodes       %011p (%u/%u)\n", nodes,             counts.nodes,                maxCounts.nodes);
    fs.formatPen("Node Kids   %011p (%u/%u)\n", nodeChildren,      counts.nodeChildren,         maxCounts.nodeChildren);
    fs.formatPen("NWeights    %011p (%u/%u)\n", nodeWeights,       counts.nodeWeights,          maxCounts.nodeWeights);
    fs.formatPen("Samplers    %011p (%u/%u)\n", samplers,          counts.samplers,             maxCounts.samplers);
    fs.formatPen("Scenes      %011p (%u/%u)\n", scenes,            counts.scenes,               maxCounts.scenes);
    fs.formatPen("Skins       %011p (%u/%u)\n", skins,             counts.skins,                maxCounts.skins);
    fs.formatPen("Textures    %011p (%u/%u)\n", textures,          counts.textures,             maxCounts.textures);
    fs.formatPen("raw data    %011p (%u/%u)\n", rawData,           counts.rawDataLen,           maxCounts.rawDataLen);

    fs.formatPen("scene       %011p (%zu)\n", scene, scene - scenes);

    fs.formatPen("copyright:  %s\n", copyright);
    fs.formatPen("generator:  %s\n", generator);
    fs.formatPen("version:    %s\n", version);
    fs.formatPen("minVersion: %s\n", minVersion);

    fs.formatPen("bounds min: %14.6f,%14.6f,%14.6f\n", bounds.min.x, bounds.min.y, bounds.min.z);
    fs.formatPen("bounds max: %14.6f,%14.6f,%14.6f\n", bounds.max.x, bounds.max.y, bounds.max.z);

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
    fs.formatPen("strings len:  %u\n", allStrLen);
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
    fs.formatPen("MWeights:     %d\n", meshWeights);
    fs.formatPen("Node:         %d\n", nodes);
    fs.formatPen("Node Kids:    %d\n", nodeChildren);
    fs.formatPen("NWeights:     %d\n", nodeWeights);
    fs.formatPen("Sampler:      %d\n", samplers);
    fs.formatPen("Scene:        %d\n", scenes);
    fs.formatPen("Skin:         %d\n", skins);
    fs.formatPen("Texture:      %d\n", textures);
    fs.formatPen("raw data size:%u\n", rawDataLen);
    fs.formatPen("Total size: %zu\n", totalSize());

    fs.terminatePen();

    return str;
}

#endif // DEBUG || DEV_INTERFACE

