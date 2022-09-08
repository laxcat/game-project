#pragma once
#include <stdint.h>

namespace gltf {
    // Philosophy: basically mirror the spec
    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
    //
    // General rules:
    // • if something is an integer reference to something else (a common pattern),
    //   use a pointer to that other object
    // • variable length arrays will need to have external data, of which the both the
    //   Loader and SizeFinder need to be aware
    // • no support for extensions yet, so leave that out
    // • no support for sparse accessors, so leave that out
    // • expect 4GB limit for buffers (for now), so favor uint32_t for buffer sizes/offsets

    // forward
    struct BufferView;
    struct Skin;
    struct AnimationChannel;
    struct AnimationSampler;
    struct Node;
    struct Sampler;

    struct Accessor {
        BufferView * bufferView;
        uint32_t byteOffset;
        int componentType;
        bool normalized;
        uint32_t count;
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
        Type type;
        float min[16];
        float max[16];
        char const * name;
    };

    struct Animation {
        AnimationChannel * channels;
        int nChannels;
        AnimationSampler * samplers;
        int nSamplers;
        char const * name;
    };

    enum AnimationTarget {
        ANIM_TAR_UNDEFINED,
        ANIM_TAR_WEIGHTS,
        ANIM_TAR_TRANSLATION,
        ANIM_TAR_ROTATION,
        ANIM_TAR_SCALE,
    };

    struct AnimationChannelTarget {
        Node * node;
        AnimationTarget path;
    };

    struct AnimationChannel {
        Sampler * sampler;
        AnimationChannelTarget target;
    };

    struct AnimationSampler {
        Accessor * input;
        enum Interpolation {
            INTERP_LINEAR,
            INTERP_STEP,
            INTERP_CUBICSPLINE
        };
        Interpolation interpolation;
        Accessor * output;
    };

    struct Asset {
        char const * copyright;
        char const * generator;
        char const * version;
        char const * minVersion;
    };

    struct Buffer {
        byte_t * data;
        uint32_t byteLength;
        char const * name;
    };

    struct BufferView {
        Buffer * buffer;
        uint32_t byteOffset;
        uint32_t byteLength;
        uint32_t byteStride;
        enum Target {
            ARRAY_BUFFER            = 0x8892, // 34962
            ELEMENT_ARRAY_BUFFER    = 0x8893, // 34963
        };
        Target target;
        char const * name;
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

    struct Camera {
        CameraOrthographic * orthographic;
        CameraPerspective * perspective;
        enum Type {
            TYPE_ORTHO,
            TYPE_PERSP,
        };
        Type type;
        char const * name;
    };

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

    struct Material {};

    struct Mesh {};

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
        int nWeights;
        char const * name;
    };

    struct Sampler {};

    struct Scene {
        int * nodes;
        int nNodes;
        char const * name;
    };

    struct Skin {};

    struct Texture {};

    struct GLTF {
        Accessor    * accessors;
        Animation   * animations;
        Asset         asset;
        Buffer      * buffers;
        BufferView  * bufferViews;
        Camera      * cameras;
        Image       * images;
        Material    * materials;
        Mesh        * meshes;
        Node        * nodes;
        Sampler     * samplers;
        Scene       * scenes;
        Skin        * skins;
        Texture     * textures;

        int16_t scene = -1;

        uint16_t nAccessors;
        uint16_t nAnimations;
        uint16_t nBuffers;
        uint16_t nBufferViews;
        uint16_t nCameras;
        uint16_t nImages;
        uint16_t nMaterials;
        uint16_t nMeshes;
        uint16_t nNodes;
        uint16_t nSamplers;
        uint16_t nScene;
        uint16_t nScenes;
        uint16_t nSkins;
        uint16_t nTextures;
    };
}
