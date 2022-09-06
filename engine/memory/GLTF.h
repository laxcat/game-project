#pragma once
#include <stdint.h>

namespace gltf {
    struct BufferView;

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

    struct Animation {};
    struct Asset {
        char const * copyright;
        char const * generator;
        char const * version;
        char const * minVersion;
    };
    struct Buffer {};
    struct BufferView {};
    struct Camera {};
    struct Image {};
    struct Material {};
    struct Mesh {};
    struct Node {};
    struct Sampler {};
    struct Scene {
        int * nodes;
        int nNodes;
        char const name[32];
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
