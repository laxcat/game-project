#pragma once
#include <mutex>
#include <stddef.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../common/AABB.h"
#include "../common/debug_defines.h"
#include "../common/types.h"

/*
Gobj (game object, "gobject")
Main structure object for items/things/characters that exist in the game world.
Mostly deals with visual data, but will probably hold physics/etc data as well.
Designed to only exist in pre-allocated space (Use GLTFLoader::calculateSize)
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


Complete memory layout below. Sections (marked with ---) are aligned to
Gobj::Align. Repeated objects indicate tight arrays with no interstitial
alignment. All layout should happen in Gobj ctor.

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
MeshAttribute
MeshAttribute
...
----------------------------------------
MeshPrimitive
MeshPrimitive
...
----------------------------------------
MeshTarget
MeshTarget
...
----------------------------------------
float                                   // Mesh weights
float
...
----------------------------------------
Node
Node
...
----------------------------------------
Node *                                  // Node children
Node *
...
----------------------------------------
float                                   // Node weights
float
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
Skin
Skin
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
// -------------------------------------------------------------------------- //
// STATIC CONSTANTS
public:
    static constexpr size_t Align = 16;

// -------------------------------------------------------------------------- //
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
    struct Mesh;
    struct MeshAttribute;
    struct MeshPrimitive;
    struct MeshTarget;
    struct Node;
    struct Sampler;
    struct Scene;
    struct Skin;
    struct Texture;

// -------------------------------------------------------------------------- //
// ENUMS, TYPES
public:
    struct Counts {
        uint32_t allStrLen          = 0;
        uint16_t accessors          = 0;
        uint16_t animations         = 0;
        uint16_t animationChannels  = 0;
        uint16_t animationSamplers  = 0;
        uint16_t buffers            = 0;
        uint16_t bufferViews        = 0;
        uint16_t cameras            = 0;
        uint16_t images             = 0;
        uint16_t materials          = 0;
        uint16_t meshes             = 0;
        uint16_t meshAttributes     = 0;
        uint16_t meshPrimitives     = 0;
        uint16_t meshTargets        = 0;
        uint16_t meshWeights        = 0;
        uint16_t nodes              = 0;
        uint16_t nodeChildren       = 0;
        uint16_t nodeWeights        = 0;
        uint16_t samplers           = 0;
        uint16_t scenes             = 0;
        uint16_t skins              = 0;
        uint16_t textures           = 0;
        uint32_t rawDataLen         = 0;

        size_t totalSize() const;

        Counts operator+(Counts const & other) const;

        #if DEBUG || DEV_INTERFACE
        void print() const;
        char * printToFrameStack() const;
        #endif // DEBUG || DEV_INTERFACE
    };

    enum Status {
        STATUS_UNINITIALIZED,
        STATUS_ERROR,
        STATUS_LOADING,
        STATUS_LOADED,
        STATUS_DECODING,
        STATUS_READY_TO_DRAW,
    };

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

    using NodeFn = std::function<void(Node *, glm::mat4 const &)>;
    using MeshFn = std::function<void(Mesh *, glm::mat4 const &)>;
    using PrimFn = std::function<void(MeshPrimitive *, glm::mat4 const &)>;
    struct TraverseFns {
        NodeFn const & eachNode = nullptr;
        MeshFn const & eachMesh = nullptr;
        PrimFn const & eachPrim = nullptr;
    };

// -------------------------------------------------------------------------- //
// INIT
private:
    friend class MemMan;
    Gobj(Counts const & counts);

// -------------------------------------------------------------------------- //
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
    MeshTarget       * meshTargets       = nullptr;
    float            * meshWeights       = nullptr;
    Node             * nodes             = nullptr;
    Node *           * nodeChildren      = nullptr;
    float            * nodeWeights       = nullptr;
    Sampler          * samplers          = nullptr;
    Scene            * scenes            = nullptr;
    Skin             * skins             = nullptr;
    Texture          * textures          = nullptr;
    byte_t           * rawData           = nullptr;

    Counts const counts;

    Scene * scene = nullptr;

    AABB bounds;

    // asset
    char const * copyright = nullptr;
    char const * generator = nullptr;
    char const * version = nullptr;
    char const * minVersion = nullptr;

    char const * loadedDirName = nullptr;

    #if DEBUG
    char const * jsonStr = nullptr;
    #endif // DEBUG

// INTERFACE
    void setStatus(Status status);
    bool isReadyToDraw() const;
    void copy(Gobj * srcGobj);
    void traverse(                          TraverseFns const & params, glm::mat4 const & parentTransform = glm::mat4{1.f});
    void traverseNode(Node * node,          TraverseFns const & params, glm::mat4 const & parentTransform = glm::mat4{1.f});
    void traverseMesh(Mesh * mesh,          TraverseFns const & params, glm::mat4 const & parentTransform = glm::mat4{1.f});


// PRIVATE STORAGE
private:
    Status _status = STATUS_UNINITIALIZED;
    mutable std::mutex _mutex;

// RELATIVE POINTER FUNCTIONS
private:
    char const       * stringRelPtr             (char const * str,              Gobj * dst) const;
    Accessor         * accessorRelPtr           (Accessor * accessor,           Gobj * dst) const;
    Animation        * animationRelPtr          (Animation * animation,         Gobj * dst) const;
    AnimationChannel * animationChannelRelPtr   (AnimationChannel * channel,    Gobj * dst) const;
    AnimationSampler * animationSamplerRelPtr   (AnimationSampler * sampler,    Gobj * dst) const;
    Buffer           * bufferRelPtr             (Buffer * buffer,               Gobj * dst) const;
    BufferView       * bufferViewRelPtr         (BufferView * bufferView,       Gobj * dst) const;
    Camera           * cameraRelPtr             (Camera * camera,               Gobj * dst) const;
    Image            * imageRelPtr              (Image * image,                 Gobj * dst) const;
    Material         * materialRelPtr           (Material * material,           Gobj * dst) const;
    Mesh             * meshRelPtr               (Mesh * mesh,                   Gobj * dst) const;
    MeshAttribute    * meshAttributeRelPtr      (MeshAttribute * meshAttribute, Gobj * dst) const;
    MeshPrimitive    * meshPrimitiveRelPtr      (MeshPrimitive * meshPrimitive, Gobj * dst) const;
    MeshTarget       * meshTargetRelPtr         (MeshTarget * meshTarget,       Gobj * dst) const;
    float            * meshWeightRelPtr         (float * meshWeight,            Gobj * dst) const;
    Node             * nodeRelPtr               (Node * node,                   Gobj * dst) const;
    Node            ** nodeChildrenRelPtr       (Node ** nodeChildren,          Gobj * dst) const;
    float            * nodeWeightRelPtr         (float * nodeWeight,            Gobj * dst) const;
    Sampler          * samplerRelPtr            (Sampler * sampler,             Gobj * dst) const;
    Scene            * sceneRelPtr              (Scene * scene,                 Gobj * dst) const;
    Skin             * skinRelPtr               (Skin * skin,                   Gobj * dst) const;
    Texture          * textureRelPtr            (Texture * texture,             Gobj * dst) const;
    byte_t           * rawDataRelPtr            (byte_t * data,                 Gobj * dst) const;

// -------------------------------------------------------------------------- //
// SUB-PARTS
public:
    struct Accessor {
        BufferView * bufferView = nullptr;
        uint32_t byteOffset = 0;
        enum ComponentType {
            COMPTYPE_BYTE           = 0x1400, // 5120
            COMPTYPE_UNSIGNED_BYTE  = 0x1401, // 5121
            COMPTYPE_SHORT          = 0x1402, // 5122
            COMPTYPE_UNSIGNED_SHORT = 0x1403, // 5123
            COMPTYPE_UNSIGNED_INT   = 0x1405, // 5125
            COMPTYPE_FLOAT          = 0x1406, // 5126
        };
        ComponentType componentType = COMPTYPE_UNSIGNED_BYTE;
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

        uint16_t renderHandle = UINT16_MAX;

        uint8_t componentCount() const;
        uint32_t byteSize() const;

        void copy(Accessor * accessor, Gobj * dst, Gobj * src);

        #if DEBUG || DEV_INTERFACE
        void print(int indent = 0) const;
        char * printToFrameStack(int indent = 0) const;
        #endif // DEBUG || DEV_INTERFACE
    };

    struct Animation {
        AnimationChannel * channels = nullptr;
        uint32_t nChannels = 0;
        AnimationSampler * samplers = nullptr;
        uint32_t nSamplers = 0;
        char const * name = nullptr;

        void copy(Animation * animation, Gobj * dst, Gobj * src);
    };

    struct AnimationChannel {
        AnimationSampler * sampler = nullptr;
        Node * node = nullptr;
        enum Target {
            TARGET_UNDEFINED,
            TARGET_WEIGHTS,
            TARGET_TRANSLATION,
            TARGET_ROTATION,
            TARGET_SCALE,
        };
        Target path = TARGET_UNDEFINED;

        void copy(AnimationChannel * channel, Gobj * dst, Gobj * src);
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

        void copy(AnimationSampler * sampler, Gobj * dst, Gobj * src);
    };

    struct Buffer {
        byte_t * data = nullptr;
        uint32_t byteLength = 0;
        char const * name = nullptr;

        void copy(Buffer * buffer, Gobj * dst, Gobj * src);
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

        void copy(BufferView * bufferView, Gobj * dst, Gobj * src);
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

        void copy(Camera * camera, Gobj * dst, Gobj * src);
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
        char const * uri = nullptr;
        char const * name = nullptr;

        void * decoded = nullptr;

        void copy(Image * image, Gobj * dst, Gobj * src);
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
            float baseColorFactor[4] = {1.f, 1.f, 1.f, 1.f};
            Texture * baseColorTexture = nullptr;
            Attr baseColorTexCoord = ATTR_TEXCOORD0;
            float metallicFactor = 1.f;
            Texture * metallicRoughnessTexture = nullptr;
            Attr metallicRoughnessTexCoord = ATTR_TEXCOORD0;
            float roughnessFactor = 1.f;
        char const * name = nullptr;

        void copy(Material * material, Gobj * dst, Gobj * src);
    };

    struct Mesh {
        MeshPrimitive * primitives = nullptr;
        int nPrimitives = 0;
        float * weights = nullptr;
        int nWeights = 0;
        char const * name = nullptr;

        void copy(Mesh * mesh, Gobj * dst, Gobj * src);
    };

    struct MeshAttribute {
        Attr type = ATTR_POSITION;
        Accessor * accessor = nullptr;

        void copy(MeshAttribute * meshAttribute, Gobj * dst, Gobj * src);

        #if DEBUG || DEV_INTERFACE
        void print(int indent = 0) const;
        char * printToFrameStack(int indent = 0) const;
        #endif // DEBUG || DEV_INTERFACE
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
        MeshTarget * targets = nullptr;
        int nTargets = 0;

        void copy(MeshPrimitive * meshPrimitive, Gobj * dst, Gobj * src);
    };

    struct MeshTarget {
        MeshAttribute * attributes = nullptr;
        int nAttributes = 0;

        void copy(MeshTarget * meshTarget, Gobj * dst, Gobj * src);
    };

    struct Node {
        Camera * camera = nullptr;
        Node ** children = nullptr;
        int nChildren = 0;
        Skin * skin = nullptr;
        glm::mat4 matrix{
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
        Mesh * mesh = nullptr;
        glm::quat rotation    {1.f, 0.f, 0.f, 0.f}; // glm::quat constructor takes wxyz!
        glm::vec3 scale       {1.f, 1.f, 1.f};
        glm::vec3 translation {0.f, 0.f, 0.f};
        float * weights = nullptr;
        int nWeights; // must match mesh weights
        char const * name = nullptr;

        void syncMatrixTRS(bool syncChildren = true);

        void copy(Node * node, Gobj * dst, Gobj * src);
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

        void copy(Sampler * sampler, Gobj * dst, Gobj * src);
    };

    struct Scene {
        Node ** nodes = nullptr; // roots nodes (spec supports multiple roots)
        int nNodes = 0;
        static constexpr size_t NameSize = 8;
        char name[NameSize] = "";

        void copy(Scene * scene, Gobj * dst, Gobj * src);
    };

    struct Skin {
        Accessor * inverseBindMatrices = nullptr;
        Node * skeleton = nullptr;
        Node ** joints = nullptr;
        int nJoints = 0;
        char const * name = nullptr;

        void copy(Skin * skin, Gobj * dst, Gobj * src);
    };

    struct Texture {
        Sampler * sampler = nullptr;
        Image * source = nullptr;
        char const * name = nullptr;

        uint16_t renderHandle = UINT16_MAX;

        void copy(Texture * texture, Gobj * dst, Gobj * src);
    };

// -------------------------------------------------------------------------- //
// STATIC HELPER CONVERSIONS
public:
    // from string
    static Accessor::Type accessorTypeFromStr(char const * str);
    static AnimationChannel::Target animationTargetFromStr(char const * str);
    static AnimationSampler::Interpolation interpolationFromStr(char const * str);
    static Camera::Type cameraTypeFromStr(char const * str);
    static Image::MIMEType imageMIMETypeFromStr(char const * str);
    static Material::AlphaMode alphaModeFromStr(char const * str);
    static Attr attrFromStr(char const * str);
    // to string
    static char const * attrStr(Gobj::Attr attr);
    // attr conversion
    static Attr texCoordAttr(int index);

// STATIC HELPERS
public:
    // generic fn that assumes invalid render handles are UINT16_MAX
    // (matches bgfx, but not-exactly-coupled)
    static bool isValid(uint16_t handle);

// -------------------------------------------------------------------------- //
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
