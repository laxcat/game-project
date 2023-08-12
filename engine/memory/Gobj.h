#pragma once

#include <stddef.h>
#include "../common/types.h"
#include "FrameStack.h"

/*

Memory layout:

|------------|----------------|----------|----------|----------|--...---|
 Gobj        FrameStack
             ^
             stringStack()
             |---- DataSize --------------------------------------------|


*/

class Gobj {
    // Philosophy: basically mirror the GLTF spec
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
    //
    // General rules:
    // • if something is an integer reference to something else (a common pattern),
    //   use a pointer to that other object
    // • variable length arrays will need to have external data, see Counter
    //   for calculation.
    // • no support for extensions yet, so leave that out
    // • no support for sparse accessors, so leave that out
    // • expect 4GB limit for buffers (for now), so favor uint32_t for buffer sizes/offsets
    // • single instance classes must exist should be packed into the parent if it makes sense.
    //   for example the PBRMetalicRoughness class is packed directly into Material.

    // forward
    struct Accessor;
    struct Animation;
    struct AnimationChannel;
    struct AnimationSampler;
    struct Buffer;
    struct BufferView;
    struct Camera;
    struct CameraOrthographic;
    struct CameraPerspective;
    struct Image;
    struct Material;
    struct MatieralNormalTexture;
    struct MatieralOcclusionTexture;
    struct Mesh;
    struct MeshAttribute;
    struct MeshPrimative;
    struct Node;
    struct Sampler;
    struct Scene;
    struct Skin;
    struct Texture;
    struct TextureInfo;

    struct Counter;

    // storage

    Accessor         * accessors         = nullptr;
    Animation        * animations        = nullptr;
    AnimationChannel * animationChannels = nullptr;
    AnimationSampler * animationSamplers = nullptr;
    Buffer           * buffers           = nullptr;
    BufferView       * bufferViews       = nullptr;
    Camera           * cameras           = nullptr;
    Image            * images            = nullptr;
    Material         * materials         = nullptr;
    Mesh             * meshes            = nullptr;
    Node             * nodes             = nullptr;
    Sampler          * samplers          = nullptr;
    Scene            * scenes            = nullptr;
    Skin             * skins             = nullptr;
    Texture          * textures          = nullptr;

    uint16_t nAccessors = 0;
    uint16_t nAnimations = 0;
    uint16_t nAnimationChannels = 0;
    uint16_t nAnimationSamplers = 0;
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

    int16_t scene = -1;

    // asset
    char const * copyright;
    char const * generator;
    char const * version;
    char const * minVersion;

    // STATIC API

    constexpr size_t DataSize(Counter const & counter) {
        return counter.totalSize();
    }

    // API

    byte_t const * data () const { return (byte_t *)this + sizeof(Gobj); }
    FrameStack const * stringStack () const { return (FrameStack *)data(); }

    // enums, types

    // matches bgfx, TODO: compare to "official" gltf supported list
    enum Attr {
        ATTR_POSITION,
        ATTR_NORMAL,
        ATTR_TANGENT,
        ATTR_BITANGENT,
        ATTR_COLOR0,
        ATTR_COLOR1,
        ATTR_COLOR2,
        ATTR_COLOR3,
        ATTR_INDICES,
        ATTR_WEIGHT,
        ATTR_TEXCOORD0,
        ATTR_TEXCOORD1,
        ATTR_TEXCOORD2,
        ATTR_TEXCOORD3,
        ATTR_TEXCOORD4,
        ATTR_TEXCOORD5,
        ATTR_TEXCOORD6,
        ATTR_TEXCOORD7,
    };
    enum AnimationTarget {
        ANIM_TAR_UNDEFINED,
        ANIM_TAR_WEIGHTS,
        ANIM_TAR_TRANSLATION,
        ANIM_TAR_ROTATION,
        ANIM_TAR_SCALE,
    };

    // structs

    struct Accessor {
        BufferView * bufferView = nullptr;
        uint32_t byteOffset = 0;
        enum ComponentType {
            COMP_BYTE           = 0x1400, // 5120
            COMP_UNSIGNED_BYTE  = 0x1401, // 5121
            COMP_SHORT          = 0x1402, // 5122
            COMP_UNSIGNED_SHORT = 0x1403, // 5123
            COMP_UNSIGNED_INT   = 0x1405, // 5125
            COMP_FLOAT          = 0x1406, // 5126
        };
        ComponentType componentType = COMP_UNSIGNED_BYTE;
        bool normalized = false;
        uint32_t count = 0;
        enum Type {
            TYPE_UNDEFINED,
            TYPE_SCALAR,
            TYPE_VEC2,
            TYPE_VEC3,
            TYPE_VEC4,
            TYPE_MAT2,
            TYPE_MAT3,
            TYPE_MAT4,
        };
        Type type = TYPE_UNDEFINED;
        float min[16] = {0.0f};
        float max[16] = {0.0f};
        char const * name = nullptr;
    };

    struct Animation {
        AnimationChannel * channels = nullptr;
        int nChannels = 0;
        AnimationSampler * samplers = nullptr;
        int nSamplers = 0;
        char const * name = nullptr;
    };

    struct AnimationChannel {
        Sampler * sampler = nullptr;
        Node * node = nullptr;
        AnimationTarget path = ANIM_TAR_UNDEFINED;
    };

    struct AnimationSampler {
        Accessor * input = nullptr;
        enum Interpolation {
            INTERP_LINEAR,
            INTERP_STEP,
            INTERP_CUBICSPLINE
        };
        Interpolation interpolation = INTERP_LINEAR;
        Accessor * output = nullptr;
    };

    struct Buffer {
        byte_t * data = nullptr;
        uint32_t byteLength = 0;
        char const * name = nullptr;
    };

    struct BufferView {
        Buffer * buffer = nullptr;
        uint32_t byteOffset = 0;
        uint32_t byteLength = 0;
        uint32_t byteStride = 0;
        enum Target {
            ARRAY_BUFFER            = 0x8892, // 34962
            ELEMENT_ARRAY_BUFFER    = 0x8893, // 34963
        };
        Target target = ARRAY_BUFFER;
        char const * name = nullptr;
    };

    struct Camera {
        CameraOrthographic * orthographic = nullptr;
        CameraPerspective * perspective = nullptr;
        enum Type {
            TYPE_ORTHO,
            TYPE_PERSP,
        };
        Type type = TYPE_PERSP;
        char const * name = nullptr;
        // pointed to by orthographic or perspective
        float _data[4] = {0.f};
    };

    struct CameraOrthographic {
        float xmag;
        float ymag;
        float zfar;
        float znear;
    };

    struct CameraPerspective {
        float aspectRatio;
        float yfov;
        float zfar;
        float znear;
    };

    // not sure about this structure... imaages, especially gltf images, will
    // be read only 99.9% of the time, so we don't really need data references.
    // should the bgfx texture handle be kept here?
    struct Image {
        byte_t * data;
        enum MIMEType {
            TYPE_JPEG,
            TYPE_PNG,
        };
        MIMEType mimeType;
        BufferView * bufferView;
        char const * name;
    };

    struct Material {
        enum AlphaMode {
            ALPHA_OPAQUE,
            ALPHA_MASK,
            ALPHA_BLEND,
        };
        AlphaMode alphaMode = ALPHA_OPAQUE;
        float alphaCutoff = 0.5f;
        float baseColorFactor[4] = {1.f};
        TextureInfo * baseColorTexture = nullptr;
        bool doubleSided = false;
        float emissiveFactor[3];
        TextureInfo * emissiveTexture = nullptr;
        float metalicFactor = 1.f;
        TextureInfo * metallicRoughnessTexture = nullptr;
        char const * name;
        MatieralNormalTexture * normalTexture;
        MatieralOcclusionTexture * occlusionTexture;
        float roughnessFactor = 1.f;
    };

    struct MatieralNormalTexture {
        Texture * index;
        Attr texCoord = ATTR_TEXCOORD0;
        float scale = 0.0f;
    };

    struct MatieralOcclusionTexture {
        Texture * index;
        Attr texCoord = ATTR_TEXCOORD0;
        float strength = 1.0f;
    };

    struct Mesh {
        MeshPrimative * primatives;
        int nPrimatives;
        float * weights;
        int nWeights;
        char const * name;
    };

    struct MeshAttribute {
        Attr type;
        Accessor * accessor;
    };

    struct MeshPrimative {
        MeshAttribute * attributes;
        int nAttributes;
        Accessor * indices;
        Material * material;
        enum Mode {
            MODE_POINTS,
            MODE_LINES,
            MODE_LINE_LOOP,
            MODE_LINE_STRIP,
            MODE_TRIANGLES,
            MODE_TRIANGLE_STRIP,
            MODE_TRIANGLE_FAN,
        };
        Mode mode = MODE_TRIANGLES;
        MeshAttribute * targets;
        int nTargets;
    };

    struct Node {
        Camera * camera;
        int * children;
        int nChildren;
        Skin * skin;
        float matrix[16];
        Mesh * mesh;
        float rotation[4];
        float scale[3];
        float translation[3];
        int * weights;
        // int nWeights; // must match mesh weights
        char const * name;
    };

    struct Sampler {
        enum Filter {
            FILTER_NEAREST                  = 0x2600, // 9728
            FILTER_LINEAR                   = 0x2601, // 9729
            FILTER_NEAREST_MIPMAP_NEAREST   = 0x2700, // 9984
            FILTER_LINEAR_MIPMAP_NEAREST    = 0x2701, // 9985
            FILTER_NEAREST_MIPMAP_LINEAR    = 0x2702, // 9986
            FILTER_LINEAR_MIPMAP_LINEAR     = 0x2703, // 9987
        };
        Filter magFilter;
        Filter minFilter;
        enum Wrap {
            WRAP_CLAMP_TO_EDGE      = 0x812f, // 33071
            WRAP_MIRRORED_REPEAT    = 0x8370, // 33648
            WRAP_REPEAT             = 0x2901, // 10497
        };
        Wrap wrapS;
        Wrap wrapT;
        char const * name;
    };

    struct Scene {
        int * nodes; // roots nodes (spec supports multiple roots)
        int nNodes;
        char const * name;
    };

    struct Skin {
        Accessor * inverseBindMatrices;
        Node * skeleton;
        Node * joints;
        int nJoints;
        char const * name;
    };

    struct Texture {
        Sampler * sampler;
        Image * source;
        char const * name;
    };

    struct TextureInfo {
        Texture * index;
        Attr texCoord;
    };

    struct Counter {
        uint16_t nAccessors = 0;
        uint16_t nAnimations = 0;
        uint16_t nAnimationChannels = 0;
        uint16_t nAnimationSamplers = 0;
        uint16_t nBuffers = 0;
        uint16_t nBufferViews = 0;
        uint16_t nCameras = 0;
        uint16_t nImages = 0;
        uint16_t nMaterials = 0;
        uint16_t nMeshes = 0;
        uint16_t nMeshPrimatives = 0;
        uint16_t nMeshAttributes = 0; // combined count of nAttributes and nTargets for all primatives
        uint16_t nNodes = 0;
        uint16_t nSamplers = 0;
        uint16_t nScenes = 0;
        uint16_t nSkins = 0;
        uint16_t nTextures = 0;

        uint32_t allStrLen = 0;
        uint32_t buffersLen = 0;

        size_t totalSize() const {
            // mirrors memory order
            return
                sizeof(Gobj) +
                sizeof(FrameStack) + allStrLen +
                sizeof(Accessor) * nAccessors +
                sizeof(Animation) * nAnimations +
                sizeof(AnimationChannel) * nAnimationChannels +
                sizeof(AnimationSampler) * nAnimationSamplers +
                sizeof(Buffer) * nBuffers +
                sizeof(BufferView) * nBufferViews +
                sizeof(Camera) * nCameras +
                sizeof(Image) * nImages +
                sizeof(Material) * nMaterials +
                sizeof(Mesh) * nMeshes +
                sizeof(MeshPrimative) * nMeshPrimatives +
                sizeof(MeshAttribute) * nMeshAttributes +
                sizeof(Node) * nNodes +
                sizeof(Sampler) * nSamplers +
                sizeof(Scene) * nScenes +
                sizeof(Skin) * nSkins +
                sizeof(Texture) * nTextures +
                buffersLen
            ;
        }
    };

    void print(Gobj const * g);
    Counter getGLTFDataSize(char const * jsonStr);
    bool load(byte_t * dst, size_t dstSize);

};
