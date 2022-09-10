#include "GLTFLoader2.h"
#include <new>
#include <assert.h>
#include <string.h>
#include "GLTF.h"
#include "../common/file_utils.h"
#include "../common/modp_b64.h"
#include "../common/string_utils.h"
#include "../dev/print.h"

#define C(OFFSET, ...) (depth && 1-(int)depth <= (OFFSET) && (OFFSET) <= 0 && crumbAt(OFFSET).matches(__VA_ARGS__))
#define CC(KEY)        (depth && crumbAt(0).matches(KEY))

using namespace gltf;
using namespace gltfutil;

GLTFLoader2::Crumb::Crumb(ObjType objType, char const * key) {
    this->objType = objType;
    setKey(key);
}

void GLTFLoader2::Crumb::setKey(char const * key) {
    if (key) {
        snprintf(this->key, MaxKeyLen, "%s", key);
    }
    else {
        this->key[0] = '\0';
    }
}

// match key?
bool GLTFLoader2::Crumb::matches(char const * key) const {
    // passed null key matches any (no check, always true)
    if (!key || key[0] == '\0') return true;
    // always false if Crumb has no key, except true when empty string is passed
    if (!hasKey()) return strEqu(key, "");
    // both passed and Crumb key exists, check if passed key contains Crumb key
    return strWithin(this->key, key);
}

// match obj type and (optionally) key?
bool GLTFLoader2::Crumb::matches(ObjType objType, char const * key) const {
    if (objType != this->objType) return false;
    return matches(key);
}

char const * GLTFLoader2::Crumb::objTypeStr() const {
    return
        (objType == TYPE_OBJ) ? "OBJ" :
        (objType == TYPE_ARR) ? "ARR" :
        (objType == TYPE_STR) ? "STR" :
        (objType == TYPE_INT) ? "INT" :
        "UNKNOWN";
}

bool GLTFLoader2::Crumb::hasKey() const { return (key[0] != '\0'); }

GLTFLoader2::GLTFLoader2(byte_t * dst, size_t dstSize, Counter const & counted) :
    _dst(dst),
    _dstSize(dstSize),
    counted(counted)
{
    gltf = new (head()) GLTF{};
    _head += sizeof(GLTF);

    strStack = new (head()) Stack{counted.allStrLen};
    _head += strStack->totalSize();

    if (counted.nAccessors) {
        gltf->accessors = (Accessor *)head();
        for (int i = 0; i < counted.nAccessors; ++i) *(gltf->accessors + i) = {};
        _head += sizeof(Accessor) * counted.nAccessors;
    }
    if (counted.nAnimations) {
        gltf->animations = (Animation *)head();
        for (int i = 0; i < counted.nAnimations; ++i) *(gltf->animations + i) = {};
        _head += sizeof(Animation) * counted.nAnimations;
        gltf->animationChannels = (AnimationChannel *)head();
        for (int i = 0; i < counted.nAnimationChannels; ++i) *(gltf->animationChannels + i) = {};
        _head += sizeof(AnimationChannel) * counted.nAnimationChannels;
        gltf->animationSamplers = (AnimationSampler *)head();
        for (int i = 0; i < counted.nAnimationSamplers; ++i) *(gltf->animationSamplers + i) = {};
        _head += sizeof(AnimationSampler) * counted.nAnimationSamplers;
    }
    if (counted.nBuffers) {
        gltf->buffers = (Buffer *)head();
        for (int i = 0; i < counted.nBuffers; ++i) *(gltf->buffers + i) = {};
        _head += sizeof(Buffer) * counted.nBuffers;
    }
    if (counted.nBufferViews) {
        gltf->bufferViews = (BufferView *)head();
        for (int i = 0; i < counted.nBufferViews; ++i) *(gltf->bufferViews + i) = {};
        _head += sizeof(BufferView) * counted.nBufferViews;
    }
    if (counted.nCameras) {
        gltf->cameras = (Camera *)head();
        // each camera will have either a persp or ortho camera
        _head += (sizeof(Camera) + sizeof(CameraPerspective)) * counted.nCameras;
        // they happen to be the same size, so it doesn't matter which, but we should assert this is true
        assert(sizeof(CameraPerspective) == sizeof(CameraOrthographic));
    }
    if (counted.nImages) {
        gltf->images = (Image *)head();
        _head += sizeof(Image) * counted.nImages;
    }
    if (counted.nMaterials) {
        gltf->materials = (Material *)head();
        _head += sizeof(Material) * counted.nMaterials;
    }
    if (counted.nMeshes) {
        gltf->meshes = (Mesh *)head();
        _head += sizeof(Mesh) * counted.nMeshes;
        _head += sizeof(MeshPrimative) * counted.nMeshPrimatives;
        _head += sizeof(MeshAttribute) * counted.nMeshAttributes;
    }
    if (counted.nNodes) {
        gltf->nodes = (Node *)head();
        _head += sizeof(Node) * counted.nNodes;
    }
    if (counted.nSamplers) {
        gltf->samplers = (Sampler *)head();
        _head += sizeof(Sampler) * counted.nSamplers;
    }
    if (counted.nScenes) {
        gltf->scenes = (Scene *)head();
        _head += sizeof(Scene) * counted.nScenes;
    }
    if (counted.nSkins) {
        gltf->skins = (Skin *)head();
        _head += sizeof(Skin) * counted.nSkins;
    }
    if (counted.nTextures) {
        gltf->textures = (Texture *)head();
        _head += sizeof(Texture) * counted.nTextures;
    }

    buffers = head();
    _head += counted.buffersLen;

    assert(_head == dstSize);
    printl("_head == dstSize == %zu", _head);
}

// push crumb onto bread crumb stack
void GLTFLoader2::push(ObjType objType) {
    assert(depth < MaxDepth);
    crumbs[depth].objType = objType;
    // copy and consume key
    crumbs[depth].setKey(key);
    key[0] = '\0';
    ++depth;
}

void GLTFLoader2::pop() {
    --depth;
}

void GLTFLoader2::captureKey(char const * key) {
    snprintf(this->key, MaxKeyLen, "%s", key);
}

GLTFLoader2::Crumb & GLTFLoader2::crumbAt(int offset) {
    assert(depth && 1-(int)depth <= offset && offset <= 0);
    return crumbs[depth-1+offset];
}

size_t GLTFLoader2::handleData(byte_t * dst, char const * str, size_t strLength) {
    // base64?
    auto isDataStr = "data:application/octet-stream;base64,";
    auto isDataLen = strlen(isDataStr);
    if (strEqualForLength(str, isDataStr, isDataLen)) {
        size_t b64StrLen = strLength - isDataLen;
        modp_b64_decode((char *)dst, (char *)str + isDataLen, b64StrLen);
        return modp_b64_decode_len(b64StrLen);
    }

    // uri to load?
    // TODO: test this
    FILE * fp = fopen(str, "r");
    if (!fp) {
        fprintf(stderr, "WARNING: Error %d opening buffer file: %s\n", ferror(fp), str);
    }
    long fileSize = getFileSize(fp);
    if (fileSize == -1) {
        fclose(fp);
        return 0;
    }
    size_t readSize = fread(dst, 1, fileSize, fp);
    // error
    if (ferror(fp) || readSize != fileSize) {
        fprintf(stderr, "Error reading file \"%s\" contents: read %zu, expecting %zu\n",
            str, readSize, fileSize);
        fclose(fp);
        return 0;
    }

    return readSize;
}

bool GLTFLoader2::Null  ()           { return true; }
bool GLTFLoader2::Int   (int i)      { return true; }
bool GLTFLoader2::Int64 (int64_t  i) { return true; }
bool GLTFLoader2::Uint64(uint64_t u) { return true; }
bool GLTFLoader2::RawNumber(char const * str, size_t length, bool copy) { return true; }

bool GLTFLoader2::Bool (bool b) {
    if (C(-2,TYPE_ARR,"accessors")) {
        if (CC("normalized")) accessor()->normalized = b;
    }
    return true;
}

bool GLTFLoader2::Uint(unsigned u) {
    push(TYPE_INT);

    if (depth == 2 && CC("scene")) gltf->scene = u;

    else if (C(-2,TYPE_ARR,"accessors")) {
        if      (CC("bufferView"))    accessor()->bufferView = gltf->bufferViews + u;
        else if (CC("byteOffset"))    accessor()->byteOffset = u;
        else if (CC("componentType")) accessor()->componentType = (Accessor::ComponentType)u;
        else if (CC("count"))         accessor()->count = u;
    }

    else if (C(-4,TYPE_ARR,"animations") && C(-2,TYPE_ARR,"channels")) {
        if (CC("sampler")) animationChannel()->sampler = gltf->samplers + u;
    }

    else if (C(-4,TYPE_ARR,"animations") && C(-2,TYPE_ARR,"samplers")) {
        if      (CC("input" )) animationSampler()->input  = gltf->accessors + u;
        else if (CC("output")) animationSampler()->output = gltf->accessors + u;
    }

    else if (C(-5,TYPE_ARR,"animations") && C(-3,TYPE_ARR,"channels") && C(-1,TYPE_OBJ,"target")) {
        if (CC("node")) animationChannel()->node = gltf->nodes + u;
    }

    else if (C(-2,TYPE_ARR,"buffers") && CC("byteLength")) buffer()->byteLength = u;

    else if (C(-2,TYPE_ARR,"bufferViews")) {
        if      (CC("buffer")) bufferView()->buffer = gltf->buffers + u;
        else if (CC("byteOffset")) bufferView()->byteOffset = u;
        else if (CC("byteLength")) bufferView()->byteLength = u;
        else if (CC("byteLength")) bufferView()->byteLength = u;
        else if (CC("byteStride")) bufferView()->byteStride = u;
        else if (CC("target")) bufferView()->target = (BufferView::Target)u;

        // else if ()
    }

    pop();
    return true;
}

bool GLTFLoader2::Double(double d) {
    if (_floatPtr) {
        *_floatPtr = (float)d;
        // printl("writing %f to %p", d, _floatPtr);
        ++_floatPtr;
    }
    return true;
}

bool GLTFLoader2::String(char const * str, size_t length, bool copy) {
    push(TYPE_STR);

    if (C(-1,TYPE_OBJ,"asset")) {
        if      (CC("copyright" )) gltf->copyright  = strStack->writeStr(str, length);
        else if (CC("generator" )) gltf->generator  = strStack->writeStr(str, length);
        else if (CC("version"   )) gltf->version    = strStack->writeStr(str, length);
        else if (CC("minVersion")) gltf->minVersion = strStack->writeStr(str, length);
    }

    else if (C(-2,TYPE_ARR,"accessors")) {
        if (CC("type")) accessor()->type = accessorTypeFromStr(str);
        if (CC("name")) accessor()->name = strStack->writeStr(str, length);
    }

    else if (C(-4,TYPE_ARR,"animations") && C(-2,TYPE_ARR,"samplers")) {
        if (CC("interpolation")) animationSampler()->interpolation = interpolationFromStr(str);
    }

    else if (C(-5,TYPE_ARR,"animations") && C(-3,TYPE_ARR,"channels") && C(-1,TYPE_OBJ,"target")) {
        if (CC("path")) animationChannel()->path = animationTargetFromStr(str);
    }

    else if (C(-2,TYPE_ARR,"buffers")) {
        if (CC("uri")) {
            size_t bytesWritten;
            if ((bytesWritten = handleData(buffers, str, length))) {
                buffer()->data = buffers;
                buffer()->byteLength = bytesWritten;
                buffers += bytesWritten;
                printl("WE DID IT! %zu", bytesWritten);
            }
        }
        else if (CC("name")) buffer()->name = strStack->writeStr(str, length);
    }

    else if (C(-2,TYPE_ARR,"bufferViews")) {
        if (CC("name")) bufferView()->name = strStack->writeStr(str, length);
    }


    pop();
    return true;
}

bool GLTFLoader2::Key(char const * str, size_t length, bool copy) {
    captureKey(str);
    return true;
}

bool GLTFLoader2::StartObject() {
    push(TYPE_OBJ);

    if (C(-3,TYPE_ARR,"animations") && C(-1,TYPE_ARR,"channels") && animation()->nChannels == 0)
        animation()->channels = animationChannel();
    if (C(-3,TYPE_ARR,"animations") && C(-1,TYPE_ARR,"samplers") && animation()->nSamplers == 0)
        animation()->samplers = animationSampler();

    return true;
}

bool GLTFLoader2::StartArray () {
    push(TYPE_ARR);
    if (C(-2,TYPE_ARR,"accessors")) {
        if      (CC("min")) _floatPtr = &accessor()->min[0];
        else if (CC("max")) _floatPtr = &accessor()->max[0];
    }
    return true;
}

bool GLTFLoader2::EndObject(size_t memberCount) {
    pop();

    if      (CC("accessors" )) ++gltf->nAccessors;
    else if (CC("animations")) ++gltf->nAnimations;
    else if (C(-2,TYPE_ARR,"animations") && C(0,TYPE_ARR,"channels")) {
        ++animation()->nChannels;
        ++gltf->nAnimationChannels;
    }
    else if (C(-2,TYPE_ARR,"animations") && C(0,TYPE_ARR,"samplers")) {
        ++animation()->nSamplers;
        ++gltf->nAnimationSamplers;
    }
    else if (CC("buffers")) ++gltf->nBuffers;
    else if (CC("bufferViews")) ++gltf->nBufferViews;

    return true;
}

bool GLTFLoader2::EndArray (size_t elementCount) {

    if (C(-2,TYPE_ARR,"accessors") && CC("min|max")) _floatPtr = nullptr;

    pop();
    return true;
}

void GLTFLoader2::printBreadcrumb() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
    }
    printl();
}

Accessor::Type GLTFLoader2::accessorTypeFromStr(char const * str) {
    if (strEqu(str, "SCALAR")) return Accessor::TYPE_SCALAR;
    if (strEqu(str, "VEC2"  )) return Accessor::TYPE_VEC2;
    if (strEqu(str, "VEC3"  )) return Accessor::TYPE_VEC3;
    if (strEqu(str, "VEC4"  )) return Accessor::TYPE_VEC4;
    if (strEqu(str, "MAT2"  )) return Accessor::TYPE_MAT2;
    if (strEqu(str, "MAT3"  )) return Accessor::TYPE_MAT3;
    if (strEqu(str, "MAT4"  )) return Accessor::TYPE_MAT4;
    return Accessor::TYPE_UNDEFINED;
}

AnimationTarget GLTFLoader2::animationTargetFromStr(char const * str) {
    if (strEqu(str, "weights"    )) return ANIM_TAR_WEIGHTS;
    if (strEqu(str, "translation")) return ANIM_TAR_TRANSLATION;
    if (strEqu(str, "rotation"   )) return ANIM_TAR_ROTATION;
    if (strEqu(str, "scale"      )) return ANIM_TAR_SCALE;
    return ANIM_TAR_UNDEFINED;
}

AnimationSampler::Interpolation GLTFLoader2::interpolationFromStr(char const * str) {
    if (strEqu(str, "STEP"       )) return AnimationSampler::INTERP_STEP;
    if (strEqu(str, "CUBICSPLINE")) return AnimationSampler::INTERP_CUBICSPLINE;
    return AnimationSampler::INTERP_LINEAR;
}


byte_t * GLTFLoader2::head() const { return _dst + _head; }

Accessor * GLTFLoader2::accessor() const { return gltf->accessors + gltf->nAccessors; }
Animation * GLTFLoader2::animation() const { return gltf->animations + gltf->nAnimations; }
AnimationChannel * GLTFLoader2::animationChannel() const { return gltf->animationChannels + gltf->nAnimationChannels; }
AnimationSampler * GLTFLoader2::animationSampler() const { return gltf->animationSamplers + gltf->nAnimationSamplers; }
Buffer * GLTFLoader2::buffer() const { return gltf->buffers + gltf->nBuffers; }
BufferView * GLTFLoader2::bufferView() const { return gltf->bufferViews + gltf->nBufferViews; }

void GLTFLoader2::checkCounts() const {
    printl("Accessors  %2d == %2d", gltf->nAccessors, counted.nAccessors);
    assert(gltf->nAccessors == counted.nAccessors);
    printl("Animations %2d == %2d", gltf->nAnimations, counted.nAnimations);
    assert(gltf->nAnimations == counted.nAnimations);
    printl("AnimationChannels %2d == %2d", gltf->nAnimationChannels, counted.nAnimationChannels);
    assert(gltf->nAnimationChannels == counted.nAnimationChannels);
    printl("AnimationSamplers %2d == %2d", gltf->nAnimationSamplers, counted.nAnimationSamplers);
    assert(gltf->nAnimationSamplers == counted.nAnimationSamplers);
}
