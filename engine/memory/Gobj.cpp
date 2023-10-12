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

Gobj::Gobj(Gobj::Counts const & counts) :
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
            head += sizeof(Accessor);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.animations) {
        animations = (Animation *)head;
        for (uint16_t i = 0; i < counts.animations; ++i) {
            new (head) Animation{};
            head += sizeof(Animation);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.animationChannels) {
        animationChannels = (AnimationChannel *)head;
        for (uint16_t i = 0; i < counts.animationChannels; ++i) {
            new (head) AnimationChannel{};
            head += sizeof(AnimationChannel);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.animationSamplers) {
        animationSamplers = (AnimationSampler *)head;
        for (uint16_t i = 0; i < counts.animationSamplers; ++i) {
            new (head) AnimationSampler{};
            head += sizeof(AnimationSampler);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.buffers) {
        buffers = (Buffer *)head;
        for (uint16_t i = 0; i < counts.buffers; ++i) {
            new (head) Buffer{};
            head += sizeof(Buffer);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.bufferViews) {
        bufferViews = (BufferView *)head;
        for (uint16_t i = 0; i < counts.bufferViews; ++i) {
            new (head) BufferView{};
            head += sizeof(BufferView);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.cameras) {
        cameras = (Camera *)head;
        for (uint16_t i = 0; i < counts.cameras; ++i) {
            new (head) Camera{};
            head += sizeof(Camera);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.images) {
        images = (Image *)head;
        for (uint16_t i = 0; i < counts.images; ++i) {
            new (head) Image{};
            head += sizeof(Image);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.materials) {
        materials = (Material *)head;
        for (uint16_t i = 0; i < counts.materials; ++i) {
            new (head) Material{};
            head += sizeof(Material);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.meshes) {
        meshes = (Mesh *)head;
        for (uint16_t i = 0; i < counts.meshes; ++i) {
            new (head) Mesh{};
            head += sizeof(Mesh);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.meshAttributes) {
        meshAttributes = (MeshAttribute *)head;
        for (uint16_t i = 0; i < counts.meshAttributes; ++i) {
            new (head) MeshAttribute{};
            head += sizeof(MeshAttribute);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.meshPrimitives) {
        meshPrimitives = (MeshPrimitive *)head;
        for (uint16_t i = 0; i < counts.meshPrimitives; ++i) {
            new (head) MeshPrimitive{};
            head += sizeof(MeshPrimitive);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.meshTargets) {
        meshTargets = (MeshTarget *)head;
        for (uint16_t i = 0; i < counts.meshTargets; ++i) {
            new (head) MeshTarget{};
            head += sizeof(MeshTarget);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.meshWeights) {
        meshWeights = (float *)head;
        for (uint16_t i = 0; i < counts.meshWeights; ++i) {
            *(float *)head = 0.f;
            head += sizeof(float);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.nodes) {
        nodes = (Node *)head;
        for (uint16_t i = 0; i < counts.nodes; ++i) {
            new (head) Node{};
            head += sizeof(Node);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.nodeChildren) {
        nodeChildren = (Node **)head;
        for (uint16_t i = 0; i < counts.nodeChildren; ++i) {
            *(Node **)head = nullptr;
            head += sizeof(Node *);
        }
        // aligns on group, not on each ptr
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.nodeWeights) {
        nodeWeights = (float *)head;
        for (uint16_t i = 0; i < counts.nodeWeights; ++i) {
            *(float *)head = 0.f;
            head += sizeof(float);
        }
        // aligns on group, not on each ptr
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.samplers) {
        samplers = (Sampler *)head;
        for (uint16_t i = 0; i < counts.samplers; ++i) {
            new (head) Sampler{};
            head += sizeof(Sampler);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.scenes) {
        scenes = (Scene *)head;
        for (uint16_t i = 0; i < counts.scenes; ++i) {
            new (head) Scene{};
            head += sizeof(Scene);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.skins) {
        skins = (Skin *)head;
        for (uint16_t i = 0; i < counts.skins; ++i) {
            new (head) Skin{};
            head += sizeof(Skin);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.textures) {
        textures = (Texture *)head;
        for (uint16_t i = 0; i < counts.textures; ++i) {
            new (head) Texture{};
            head += sizeof(Texture);
        }
        head = (byte_t *)ALIGN_PTR(head);
    }

    if (counts.buffers) {
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
    memcpy(strings, src->strings, src->strings->totalSize());
    for (uint16_t i = 0; i < src->counts.accessors && i < counts.accessors; ++i) {
        accessors[i].copy(src->accessors + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animations && i < counts.animations; ++i) {
        animations[i].copy(src->animations + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animationChannels && i < counts.animationChannels; ++i) {
        animationChannels[i].copy(src->animationChannels + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.animationSamplers && i < counts.animationSamplers; ++i) {
        animationSamplers[i].copy(src->animationSamplers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.buffers && i < counts.buffers; ++i) {
        buffers[i].copy(src->buffers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.bufferViews && i < counts.bufferViews; ++i) {
        bufferViews[i].copy(src->bufferViews + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.cameras && i < counts.cameras; ++i) {
        cameras[i].copy(src->cameras + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.images && i < counts.images; ++i) {
        images[i].copy(src->images + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.materials && i < counts.materials; ++i) {
        materials[i].copy(src->materials + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshes && i < counts.meshes; ++i) {
        meshes[i].copy(src->meshes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshAttributes && i < counts.meshAttributes; ++i) {
        meshAttributes[i].copy(src->meshAttributes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshPrimitives && i < counts.meshPrimitives; ++i) {
        meshPrimitives[i].copy(src->meshPrimitives + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.meshTargets && i < counts.meshTargets; ++i) {
        meshTargets[i].copy(src->meshTargets + i, this, src);
    }
    memcpy(meshWeights, src->meshWeights, sizeof(float) * min(src->counts.meshWeights, counts.meshWeights));
    for (uint16_t i = 0; i < src->counts.nodes && i < counts.nodes; ++i) {
        nodes[i].copy(src->nodes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.nodeChildren && i < counts.nodeChildren; ++i) {
        nodeChildren[i] = src->nodeRelPtr(src->nodeChildren[i], this);
    }
    memcpy(nodeWeights, src->nodeWeights, sizeof(float) * min(src->counts.nodeWeights, counts.nodeWeights));
    for (uint16_t i = 0; i < src->counts.samplers && i < counts.samplers; ++i) {
        samplers[i].copy(src->samplers + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.scenes && i < counts.scenes; ++i) {
        scenes[i].copy(src->scenes + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.skins && i < counts.skins; ++i) {
        skins[i].copy(src->skins + i, this, src);
    }
    for (uint16_t i = 0; i < src->counts.textures && i < counts.textures; ++i) {
        textures[i].copy(src->textures + i, this, src);
    }
    memcpy(rawData, src->rawData, min(src->counts.rawDataLen, counts.rawDataLen));
    // counts = src->counts; // don't copy the counts! Gobj ctor requires counts and gets set permenantly.
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
    memcpy(name, scene->name, Scene::NameSize);
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
            traverseMesh(meshes + meshIndex, fns, parentTransform);
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
        traverseMesh(node->mesh, fns, global);
    }
    // traverse child nodes
    for (uint16_t nodeIndex = 0; nodeIndex < node->nChildren; ++nodeIndex) {
        traverseNode(node->children[nodeIndex], fns, global);
    }
}

void Gobj::traverseMesh(Mesh * mesh, TraverseFns const & fns, glm::mat4 const & parentTransform) {
    // run mesh fn if present
    if (fns.eachMesh) {
        fns.eachMesh(mesh, parentTransform);
    }
    for (int primIndex = 0; primIndex < mesh->nPrimitives; ++primIndex) {
        Gobj::MeshPrimitive * prim = mesh->primitives + primIndex;
        if (fns.eachPrim) {
            fns.eachPrim(prim, parentTransform);
        }
        if (fns.eachPosAccr) {
            for (int attrIndex = 0; attrIndex < prim->nAttributes; ++attrIndex) {
                Gobj::MeshAttribute * attr = prim->attributes + attrIndex;
                if (attr->type == Gobj::ATTR_POSITION) {
                    fns.eachPosAccr(attr->accessor, parentTransform);
                }
            }
        }
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

    ret.allStrLen          += other.allStrLen;
    ret.accessors          += other.accessors;
    ret.animations         += other.animations;
    ret.animationChannels  += other.animationChannels;
    ret.animationSamplers  += other.animationSamplers;
    ret.buffers            += other.buffers;
    ret.bufferViews        += other.bufferViews;
    ret.cameras            += other.cameras;
    ret.images             += other.images;
    ret.materials          += other.materials;
    ret.meshes             += other.meshes;
    ret.meshAttributes     += other.meshAttributes;
    ret.meshPrimitives     += other.meshPrimitives;
    ret.meshTargets        += other.meshTargets;
    ret.meshWeights        += other.meshWeights;
    ret.nodes              += other.nodes;
    ret.nodeChildren       += other.nodeChildren;
    ret.nodeWeights        += other.nodeWeights;
    ret.samplers           += other.samplers;
    ret.scenes             += other.scenes;
    ret.skins              += other.skins;
    ret.textures           += other.textures;
    ret.rawDataLen         += other.rawDataLen;

    return ret;
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
uint32_t Gobj::Accessor::byteSize() const {
    switch (componentType) {
    case COMPTYPE_BYTE           :
    case COMPTYPE_UNSIGNED_BYTE  : return componentCount() * 1;
    case COMPTYPE_SHORT          :
    case COMPTYPE_UNSIGNED_SHORT : return componentCount() * 2;
    case COMPTYPE_UNSIGNED_INT   :
    case COMPTYPE_FLOAT          : return componentCount() * 4;
    }
}

#if DEBUG || DEV_INTERFACE
void Gobj::Accessor::print(int indent) const { ::print(printToFrameStack(indent)); }
char * Gobj::Accessor::printToFrameStack(int indent) const {
    assert(mm.frameStack && "Frame stack not initialized.");

    auto data = bufferView->buffer->data + byteOffset + bufferView->byteOffset;
    auto dataEnd = bufferView->buffer->data + bufferView->byteLength;
    assert(componentType == Gobj::Accessor::COMPTYPE_FLOAT);

    FrameStack & fs = *mm.frameStack;
    char * str = (char *)fs.dataHead();

    fs.formatPen("%*sAccessor: %s (%p)\n", indent,"", name, this);
    fs.formatPen("%*s    count: %u\n", indent,"", count);
    fs.formatPen("%*s    bufferView byteLength: %u\n", indent,"", bufferView->byteLength);
    fs.formatPen("%*s    data:\n", indent,"");

    uint8_t cc = componentCount();
    for (uint32_t i = 0; i < count; ++i) {
        float * f = (float *)data;
        fs.formatPen("%*s%03u: ", indent+4,"", i);
        for (uint8_t c = 0; c < cc; ++c) {
            fs.formatPen("%+f ", f[c]);
        }
        fs.formatPen("   (%p)\n", data);
        data += bufferView->byteStride;
    }

    fs.terminatePen();
    return str;
}
#endif // DEBUG || DEV_INTERFACE

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
        matrix = glm::mat4{1.f};
        matrix = glm::translate(matrix, translation);
        matrix *= glm::mat4_cast(rotation);
        matrix = glm::scale(matrix, scale);
    }
    else {
        // matrix -> TRS
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    }

    // process children too?
    if (syncChildren) {
        for (uint16_t nodeIndex = 0; nodeIndex < nChildren; ++nodeIndex) {
            children[nodeIndex]->syncMatrixTRS(syncChildren);
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

char const * Gobj::attrStr(Gobj::Attr attr) {
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

    fs.formatPen("strings     %011p (%u)\n", strings,           counts.allStrLen);
    fs.formatPen("Accessors   %011p (%u)\n", accessors,         counts.accessors);
    fs.formatPen("Animation   %011p (%u)\n", animations,        counts.animations);
    fs.formatPen("AChannels   %011p (%u)\n", animationChannels, counts.animationChannels);
    fs.formatPen("ASamplers   %011p (%u)\n", animationSamplers, counts.animationSamplers);
    fs.formatPen("Buffers     %011p (%u)\n", buffers,           counts.buffers);
    fs.formatPen("BufferViews %011p (%u)\n", bufferViews,       counts.bufferViews);
    fs.formatPen("Cameras     %011p (%u)\n", cameras,           counts.cameras);
    fs.formatPen("Images      %011p (%u)\n", images,            counts.images);
    fs.formatPen("Materials   %011p (%u)\n", materials,         counts.materials);
    fs.formatPen("Meshs       %011p (%u)\n", meshes,            counts.meshes);
    fs.formatPen("MAttributes %011p (%u)\n", meshAttributes,    counts.meshAttributes);
    fs.formatPen("MPrimitives %011p (%u)\n", meshPrimitives,    counts.meshPrimitives);
    fs.formatPen("MTargets    %011p (%u)\n", meshTargets,       counts.meshTargets);
    fs.formatPen("MWeights    %011p (%u)\n", meshWeights,       counts.meshWeights);
    fs.formatPen("Nodes       %011p (%u)\n", nodes,             counts.nodes);
    fs.formatPen("Node Kids   %011p (%u)\n", nodeChildren,      counts.nodeChildren);
    fs.formatPen("NWeights    %011p (%u)\n", nodeWeights,       counts.nodeWeights);
    fs.formatPen("Samplers    %011p (%u)\n", samplers,          counts.samplers);
    fs.formatPen("Scenes      %011p (%u)\n", scenes,            counts.scenes);
    fs.formatPen("Skins       %011p (%u)\n", skins,             counts.skins);
    fs.formatPen("Textures    %011p (%u)\n", textures,          counts.textures);
    fs.formatPen("raw data    %011p (%u)\n", rawData,           counts.rawDataLen);

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

