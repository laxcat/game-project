#pragma once

#include <stddef.h>
#include "../common/debug_defines.h"
#include "../common/types.h"

/*
Gobj (game object, "gobject")
Main structure object for items/things/characters that exist in the game world.
Mostly deals with visual data, but will probably hold physics/etc data as well.
Designed to only exist in pre-allocated space (Use GLTFLoader4::calculateSize)
to determine size beforehand.

Visual data closely mirrors the GLTF spec.
https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
Significant exceptions:
    No extensions


Brief memory layout:

|-------|-----------|------|---------...-|---------...-|-...-------|-----------|
 Gobj   FrameStack  frame  Accessors...  Animations...   all sub-   raw buffer
                    stack                                parts
                    data                                 (Node,
        ^                  ^             ^               Mesh, etc)
        strings            accessors     animations
        data()
        |---- DataSize --------------------------------------------------------|


Complete memory layout:

----------------------------------------
Gobj
----------------------------------------
FrameStack
FrameStack string data
----------------------------------------
Accessor
Accessor
...
----------------------------------------
Animation
Animation
...
----------------------------------------
AnimationChannel
AnimationChannel
...
----------------------------------------
AnimationSampler
AnimationSampler
...
----------------------------------------
Buffer
Buffer
...
----------------------------------------
BufferView
BufferView
...
----------------------------------------
Camera
Camera
...
----------------------------------------
Image
Image
...
----------------------------------------
Material
Material
...
----------------------------------------
Mesh
Mesh
...
----------------------------------------
Node
Node
...
----------------------------------------
Sampler
Sampler
...
----------------------------------------
Scene
Scene
...
----------------------------------------
Node *                                  // Scene root nodes
Node *
...
----------------------------------------
Skin
Skin
...
----------------------------------------
Scene
Scene
...
----------------------------------------
Texture
Texture
...
----------------------------------------
raw data                                // buffer data, image data, etc.
                                        // anything that loaded externally or from base64 data.
                                        // for now, just buffer data, as images are not loaded
                                        // to main memory.
----------------------------------------

*/

// forward
class FrameStack;

class Gobj {
    // STATIC CONSTANTS
public:
    static constexpr size_t Align = 16;
    // FORWARD
public:
    // sub-parts
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
    struct MeshPrimitive;
    struct Node;
    struct Sampler;
    struct Scene;
    struct Skin;
    struct Texture;
    struct TextureInfo;

    // COUNTS
    struct Counts {
        uint16_t accessors = 0;
        uint16_t animations = 0;
        uint16_t animationChannels = 0;
        uint16_t animationSamplers = 0;
        uint16_t buffers = 0;
        uint16_t bufferViews = 0;
        uint16_t cameras = 0;
        uint16_t images = 0;
        uint16_t materials = 0;
        uint16_t meshes = 0;
        uint16_t meshPrimitives = 0;
        uint16_t meshAttributes = 0; // combined count of nAttributes and nTargets for all primitives
        uint16_t nodes = 0;
        uint16_t samplers = 0;
        uint16_t scenes = 0;
        uint16_t sceneNodes = 0;
        uint16_t skins = 0;
        uint16_t textures = 0;

        uint32_t allStrLen = 0;
        uint32_t rawDataLen = 0;

        size_t totalSize() const;
        #if DEBUG || DEV_INTERFACE
        void print() const;
        char * printToFrameStack() const;
        #endif // DEBUG || DEV_INTERFACE
    };


    // STORAGE
public:
    FrameStack       * strings           = nullptr;
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
    MeshAttribute    * meshAttributes    = nullptr;
    MeshPrimitive    * meshPrimitives    = nullptr;
    Node             * nodes             = nullptr;
    Sampler          * samplers          = nullptr;
    Scene            * scenes            = nullptr;
    Node *           * sceneNodes        = nullptr;
    Skin             * skins             = nullptr;
    Texture          * textures          = nullptr;
    byte_t           * rawData           = nullptr;

    Counts counts;

    int16_t scene = -1;

    // asset
    char const * copyright = nullptr;
    char const * generator = nullptr;
    char const * version = nullptr;
    char const * minVersion = nullptr;

    #if DEBUG
    char const * jsonStr = nullptr;
    #endif // DEBUG

    // STATIC API

    constexpr size_t DataSize(Counts const & counts) {
        return counts.totalSize();
    }

    // API
public:
    Gobj(Counts const & counts);
    byte_t const * data () const;
    FrameStack const * stringStack () const;

    // ENUMS, TYPES
public:
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

    // SUB-PARTS

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
        uint32_t nChannels = 0;
        AnimationSampler * samplers = nullptr;
        uint32_t nSamplers = 0;
        char const * name = nullptr;
    };

    struct AnimationChannel {
        AnimationSampler * sampler = nullptr;
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

    struct Image {
        enum MIMEType {
            TYPE_JPEG,
            TYPE_PNG,
        };
        MIMEType mimeType = TYPE_PNG;
        BufferView * bufferView = nullptr;
        char const * name = nullptr;
    };

    struct Material {
        enum AlphaMode {
            ALPHA_OPAQUE,
            ALPHA_MASK,
            ALPHA_BLEND,
        };
        AlphaMode alphaMode = ALPHA_OPAQUE;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
        Texture * emissiveTexture = nullptr;
        Attr emissiveTexCoord = ATTR_TEXCOORD0;
        float emissiveFactor[3] = {0.f};
        // normalTexture
            Texture * normalTexture = nullptr;
            Attr normalTexCoord = ATTR_TEXCOORD0;
            float normalScale = 1.f;
        // occlusionTexture
            Texture * occlusionTexture = nullptr;
            Attr occlusionTexCoord = ATTR_TEXCOORD0;
            float occlusionStrength = 1.f;
        // pbrMetallicRoughness
            float baseColorFactor[4] = {1.f};
            Texture * baseColorTexture = nullptr;
            Attr baseColorTexCoord = ATTR_TEXCOORD0;
            float metallicFactor = 1.f;
            Texture * metallicRoughnessTexture = nullptr;
            Attr metallicRoughnessTexCoord = ATTR_TEXCOORD0;
            float roughnessFactor = 1.f;
        char const * name = nullptr;
    };

    struct Mesh {
        MeshPrimitive * primitives = nullptr;
        int nPrimitives = 0;
        float * weights = nullptr;
        int nWeights = 0;
        char const * name = nullptr;
    };

    struct MeshAttribute {
        Attr type = ATTR_POSITION;
        Accessor * accessor = nullptr;
    };

    struct MeshPrimitive {
        MeshAttribute * attributes = nullptr;
        int nAttributes = 0;
        Accessor * indices = nullptr;
        Material * material = nullptr;
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
        MeshAttribute * targets = nullptr;
        int nTargets = 0;
    };

    struct Node {
        Camera * camera = nullptr;
        Node * children = nullptr;
        int nChildren = 0;
        Skin * skin = nullptr;
        float matrix[16] = {0.f};
        Mesh * mesh = nullptr;
        float rotation[4] = {0.f};
        float scale[3] = {0.f};
        float translation[3] = {0.f};
        int * weights = nullptr;
        // int nWeights; // must match mesh weights
        char const * name = nullptr;
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
        Filter magFilter = FILTER_LINEAR;
        Filter minFilter = FILTER_LINEAR;
        enum Wrap {
            WRAP_CLAMP_TO_EDGE      = 0x812f, // 33071
            WRAP_MIRRORED_REPEAT    = 0x8370, // 33648
            WRAP_REPEAT             = 0x2901, // 10497
        };
        Wrap wrapS = WRAP_CLAMP_TO_EDGE;
        Wrap wrapT = WRAP_CLAMP_TO_EDGE;
        char const * name = nullptr;
    };

    struct Scene {
        Node ** nodes = nullptr; // roots nodes (spec supports multiple roots)
        int nNodes = 0;
        char const * name = nullptr;
    };

    struct Skin {
        Accessor * inverseBindMatrices = nullptr;
        Node * skeleton = nullptr;
        Node * joints = nullptr;
        int nJoints = 0;
        char const * name = nullptr;
    };

    struct Texture {
        Sampler * sampler = nullptr;
        Image * source = nullptr;
        char const * name = nullptr;
    };

    // DEBUG
public:
    #if DEBUG || DEV_INTERFACE
    void print() const;
    char * printToFrameStack() const;
    #endif // DEBUG || DEV_INTERFACE

    #if DEV_INTERFACE
    static void editorCreate();
    void drawNode(Node * n);
    void editorEditBlock();
    #endif // DEV_INTERFACE
};
