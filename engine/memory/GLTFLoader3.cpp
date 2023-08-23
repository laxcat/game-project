#include "GLTFLoader3.h"
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>
#include <rapidjson/prettywriter.h>
#include "../common/file_utils.h"
#include "../common/modp_b64.h"
#include "FrameStack.h"
#include "../MrManager.h"


GLTFLoader3::SizeFinder::Crumb::Crumb(
    ObjType objType,
    char const * key
) {
    this->objType = objType;
    setKey(key);
}

GLTFLoader3::SizeFinder::Crumb::Crumb() {}

void GLTFLoader3::SizeFinder::Crumb::setKey(char const * key) {
    if (key) {
        fixstrcpy<MaxKeyLen>(this->key, key);
    }
    else {
        this->key[0] = '\0';
    }
}

char const * GLTFLoader3::SizeFinder::Crumb::objTypeStr() const {
    return
        (objType == OBJ) ? "OBJ" :
        (objType == ARR) ? "ARR" :
        (objType == STR) ? "STR" :
        (objType == INT) ? "INT" :
        "UNKNOWN";
}

bool GLTFLoader3::SizeFinder::Crumb::hasKey() const {
    return (key[0] != '\0');
}

void GLTFLoader3::SizeFinder::ConditionGroup::push(
    int offset,
    Crumb::ObjType objType,
    char const * key,
    Action action
) {
    assert(count < MaxConditions && "MaxConditions reached");
    auto & c = all[count];
    c.offset = offset;
    c.crumb.objType = objType;
    c.crumb.setKey(key);
    c.action = action;
    ++count;
}

GLTFLoader3::SizeFinder::SizeFinder() {
    objCountConds.push(0, Crumb::ARR, "accessors",  [this](void * ud){ cntr.accessors =   (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "animations", [this](void * ud){ cntr.animations =  (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "buffers",    [this](void * ud){ cntr.buffers =     (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "bufferViews",[this](void * ud){ cntr.bufferViews = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "cameras",    [this](void * ud){ cntr.cameras =     (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "images",     [this](void * ud){ cntr.images =      (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "materials",  [this](void * ud){ cntr.materials =   (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "meshes",     [this](void * ud){ cntr.meshes =      (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "nodes",      [this](void * ud){ cntr.nodes =       (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "samplers",   [this](void * ud){ cntr.samplers =    (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "scenes",     [this](void * ud){ cntr.scenes =      (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "skins",      [this](void * ud){ cntr.skins =       (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "textures",   [this](void * ud){ cntr.textures =    (size_t)ud; });

    // animation sub objects
    animChannelCountConds.allAction = [this](void * ud) { cntr.animationChannels += (size_t)ud; };
    animChannelCountConds.push( 0, Crumb::ARR, "channels");
    animChannelCountConds.push(-2, Crumb::ARR, "animations");
    animSamplerCountConds.allAction = [this](void * ud) { cntr.animationSamplers += (size_t)ud; };
    animSamplerCountConds.push( 0, Crumb::ARR, "samplers");
    animSamplerCountConds.push(-2, Crumb::ARR, "animations");

    // mesh sub objects
    meshPrimCountConds.allAction = [this](void * ud) { cntr.meshPrimatives += (size_t)ud; };
    meshPrimCountConds.push( 0, Crumb::ARR, "primitives");
    meshPrimCountConds.push(-2, Crumb::ARR, "meshes");
    // TODO: add morph targets to same nMeshAttributes count
    meshAttrCountConds.allAction = [this](void * ud) { cntr.meshAttributes += (size_t)ud; };
    meshAttrCountConds.push( 0, Crumb::OBJ, "attributes");
    meshAttrCountConds.push(-2, Crumb::ARR, "primitives");
    meshAttrCountConds.push(-4, Crumb::ARR, "meshes");

    // find name strings
    // find main object names
    strCountConds[0].allAction = [this](void * ud) { ++stringCounter; cntr.allStrLen += (size_t)ud; };
    strCountConds[0].push(0, Crumb::STR, "name");
    strCountConds[0].push(
        -2,
        Crumb::ARR,
        "accessors|animations|buffers|bufferViews|cameras|images|materials|meshes|"
        "nodes|samplers|scenes|skins|textures"
    );
    // find asset names
    strCountConds[1].allAction = strCountConds[0].allAction;
    strCountConds[1].push(0, Crumb::STR, "copyright|generator|version|minVersion");
    strCountConds[1].push(-1, Crumb::OBJ, "asset");

    // find buffer sizes
    byteCountConds.allAction = [this](void * ud) { cntr.buffersLen += (size_t)ud; };
    byteCountConds.push(0, Crumb::INT, "byteLength");
    byteCountConds.push(-2, Crumb::ARR, "buffers");


    // find image sizes
    // (unnecessary)
    // imageSizeConds.push(0, Crumb::STR, "uri");
    // imageSizeConds.push(-2, Crumb::ARR, "images");
    // imageSizeConds.allAction = [this](void * ud) {
    //     auto fs = (FixedStr *)ud;
    //     auto id = loadImageBase64(fs->str, fs->length);
    //     printl("image data: %dx%d, %d = %zu", id.width, id.height, id.nChannels, id.dataSize());
    // };
}

// push crumb onto bread crumb stack
void GLTFLoader3::SizeFinder::push(Crumb::ObjType objType) {
    assert(depth < MaxDepth);
    crumbs[depth].objType = objType;
    // copy and consume key
    crumbs[depth].setKey(key);
    key[0] = '\0';
    ++depth;
}

void GLTFLoader3::SizeFinder::pop() {
    --depth;
}

void GLTFLoader3::SizeFinder::captureKey(char const * key) {
    fixstrcpy<MaxKeyLen>(this->key, key);
}

// check condition against current state
bool GLTFLoader3::SizeFinder::check(Condition const & con, void * userData) {
    // offset relative to last (most recent) crumb in stack
    auto & actualCrumb  = crumbs[depth-1+con.offset];

    // printl("check %s against %s", con.crumb.objTypeStr(), actualCrumb.objTypeStr());
    if (con.crumb.objType != actualCrumb.objType) return false;
    // printl("check (%s) against (%s)", con.crumb.key, actualCrumb.key);
    if (con.crumb.hasKey() && !strWithin(actualCrumb.key, con.crumb.key, '|')) return false;
    if (con.action) con.action(userData);
    // printl("TRUE!");
    return true;
}

// checks each condition in group (never short circuts).
// returns true if ANY condition was true.
bool GLTFLoader3::SizeFinder::checkEach(ConditionGroup const & group, void * userData) {
    bool ret = false;
    for (int i = 0; i < group.count; ++i) {
        ret = ret || check(group.all[i], userData);
    }
    return ret;
}

// checks combination of all conditions in group (short circuts at first false).
// returns true only if ALL conditions were true.
bool GLTFLoader3::SizeFinder::checkAll(ConditionGroup const & group, void * userData) {
    // printl("check all user data: %zu", (size_t)userData);
    for (int i = 0; i < group.count; ++i) {
        if (!check(group.all[i], userData)) return false;
    }
    if (group.allAction) group.allAction(userData);
    return true;
}

bool GLTFLoader3::SizeFinder::Null  ()           { return true; }
bool GLTFLoader3::SizeFinder::Bool  (bool b)     { return true; }
bool GLTFLoader3::SizeFinder::Int   (int i)      { return true; }
bool GLTFLoader3::SizeFinder::Int64 (int64_t  i) { return true; }
bool GLTFLoader3::SizeFinder::Uint64(uint64_t u) { return true; }
bool GLTFLoader3::SizeFinder::Double(double   d) { return true; }
bool GLTFLoader3::SizeFinder::RawNumber(char const * str, size_t length, bool copy) { return true; }

bool GLTFLoader3::SizeFinder::Uint(unsigned u) {
    push(Crumb::INT);
    checkAll(byteCountConds, (void *)(size_t)u);
    pop();
    return true;
}

bool GLTFLoader3::SizeFinder::String(char const * str, size_t length, bool copy) {
    push(Crumb::STR);

    for (int i = 0; i < nFindStrConds; ++i) {
        checkAll(strCountConds[i], (void *)(length + 1));
    }

    // special check for image size
    auto fs = FixedStr{str, length};
    checkAll(imageSizeConds, (void *)&fs);

    pop();
    return true;
}

bool GLTFLoader3::SizeFinder::Key(char const * str, size_t length, bool copy) {
    captureKey(str);
    return true;
}

bool GLTFLoader3::SizeFinder::StartObject() {
    push(Crumb::OBJ);
    // printBreadcrumb();
    return true;
}

bool GLTFLoader3::SizeFinder::StartArray () {
    push(Crumb::ARR);
    // printBreadcrumb();
    return true;
}

bool GLTFLoader3::SizeFinder::EndObject(size_t memberCount) {
    checkAll(meshAttrCountConds, (void *)memberCount);
    pop();
    return true;
}

bool GLTFLoader3::SizeFinder::EndArray (size_t elementCount) {
    checkEach(objCountConds, (void *)elementCount);
    checkAll(animChannelCountConds, (void *)elementCount);
    checkAll(animSamplerCountConds, (void *)elementCount);
    checkAll(meshPrimCountConds, (void *)elementCount);
    pop();
    // printBreadcrumb();
    return true;
}

Gobj::Counts const & GLTFLoader3::SizeFinder::counter() const { return cntr; }

size_t GLTFLoader3::SizeFinder::totalSize() const { return cntr.totalSize(); }

void GLTFLoader3::SizeFinder::printBreadcrumb() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
    }
    printl();
}

void GLTFLoader3::SizeFinder::printStats() const {
    printl("Accessors:    %8u", cntr.accessors);
    printl("Animations:   %8u", cntr.animations);
    printl("Anim Channels:%8u", cntr.animationChannels);
    printl("Anim Samplers:%8u", cntr.animationSamplers);
    printl("Buffers:      %8u", cntr.buffers);
    printl("BufferViews:  %8u", cntr.bufferViews);
    printl("Cameras:      %8u", cntr.cameras);
    printl("Images:       %8u", cntr.images);
    printl("Materials:    %8u", cntr.materials);
    printl("Meshes:       %8u", cntr.meshes);
    printl("Mesh prims:   %8u", cntr.meshPrimatives);
    printl("Mesh attrs:   %8u", cntr.meshAttributes);
    printl("Nodes:        %8u", cntr.nodes);
    printl("Samplers:     %8u", cntr.samplers);
    printl("Scenes:       %8u", cntr.scenes);
    printl("Skins:        %8u", cntr.skins);
    printl("Textures:     %8u", cntr.textures);
    printl("Strings len:  %8u", cntr.allStrLen);
    printl("String count: %8u", stringCounter);
    printl("Buffers len:  %8u", cntr.buffersLen);
    printl("Total Size:   %8zu", cntr.totalSize());
}



// =============================================================================
// =============================================================================
// =============================================================================




#define C(OFFSET, ...) (depth && 1-(int)depth <= (OFFSET) && (OFFSET) <= 0 && crumbAt(OFFSET).matches(__VA_ARGS__))
#define CC(KEY)        (depth && crumbAt(0).matches(KEY))

GLTFLoader3::Loader::Crumb::Crumb(ObjType objType, char const * key) {
    this->objType = objType;
    setKey(key);
}

void GLTFLoader3::Loader::Crumb::setKey(char const * key) {
    if (key) {
        fixstrcpy<MaxKeyLen>(this->key, key);
    }
    else {
        this->key[0] = '\0';
    }
}

// match key?
bool GLTFLoader3::Loader::Crumb::matches(char const * key) const {
    // passed null key matches any (no check, always true)
    if (!key || key[0] == '\0') return true;
    // always false if Crumb has no key, except true when empty string is passed
    if (!hasKey()) return strEqu(key, "");
    // both passed and Crumb key exists, check if passed key contains Crumb key
    return strWithin(this->key, key);
}

// match obj type and (optionally) key?
bool GLTFLoader3::Loader::Crumb::matches(ObjType objType, char const * key) const {
    if (objType != this->objType) return false;
    return matches(key);
}

char const * GLTFLoader3::Loader::Crumb::objTypeStr() const {
    return
        (objType == TYPE_OBJ)   ? "OBJ" :
        (objType == TYPE_ARR)   ? "ARR" :
        (objType == TYPE_STR)   ? "STR" :
        (objType == TYPE_UINT)  ? "UINT" :
        (objType == TYPE_INT)   ? "INT" :
        (objType == TYPE_FLOAT) ? "FLOAT" :
        "UNKNOWN";
}

bool GLTFLoader3::Loader::Crumb::hasKey() const { return (key[0] != '\0'); }

GLTFLoader3::Loader::Loader(byte_t * dst, size_t dstSize, Gobj::Counts const & counts) :
    _dst(dst),
    _dstSize(dstSize),
    counts(counts)
{
    // debugBreak();
    
    gobj = new (head()) Gobj{counts};
    _head += sizeof(Gobj);

    strStack = new (head()) FrameStack{counts.allStrLen};
    _head += strStack->totalSize();

    if (counts.accessors) {
        gobj->accessors = (Gobj::Accessor *)head();
        for (int i = 0; i < counts.accessors; ++i) *(gobj->accessors + i) = {};
        _head += sizeof(Gobj::Accessor) * counts.accessors;
    }
    if (counts.animations) {
        gobj->animations = (Gobj::Animation *)head();
        for (int i = 0; i < counts.animations; ++i) *(gobj->animations + i) = {};
        _head += sizeof(Gobj::Animation) * counts.animations;
        gobj->animationChannels = (Gobj::AnimationChannel *)head();
        for (int i = 0; i < counts.animationChannels; ++i) *(gobj->animationChannels + i) = {};
        _head += sizeof(Gobj::AnimationChannel) * counts.animationChannels;
        gobj->animationSamplers = (Gobj::AnimationSampler *)head();
        for (int i = 0; i < counts.animationSamplers; ++i) *(gobj->animationSamplers + i) = {};
        _head += sizeof(Gobj::AnimationSampler) * counts.animationSamplers;
    }
    if (counts.buffers) {
        gobj->buffers = (Gobj::Buffer *)head();
        for (int i = 0; i < counts.buffers; ++i) *(gobj->buffers + i) = {};
        _head += sizeof(Gobj::Buffer) * counts.buffers;
    }
    if (counts.bufferViews) {
        gobj->bufferViews = (Gobj::BufferView *)head();
        for (int i = 0; i < counts.bufferViews; ++i) *(gobj->bufferViews + i) = {};
        _head += sizeof(Gobj::BufferView) * counts.bufferViews;
    }
    if (counts.cameras) {
        gobj->cameras = (Gobj::Camera *)head();
        for (int i = 0; i < counts.cameras; ++i) *(gobj->cameras + i) = {};
        _head += sizeof(Gobj::Camera) * counts.cameras;
        // either Gobj::Camera::perspective or Gobj::Camera::orthographic points to Gobj::Camera::_data
        // explointed fact that Gobj::CameraPerspective or Gobj::CameraOrthographic happen to both be 4 floats
        assert(sizeof(Gobj::CameraPerspective) == sizeof(Gobj::CameraOrthographic));
        assert(sizeof(Gobj::CameraPerspective) == sizeof(Gobj::Camera::_data));
    }
    if (counts.images) {
        gobj->images = (Gobj::Image *)head();
        _head += sizeof(Gobj::Image) * counts.images;
    }
    if (counts.materials) {
        gobj->materials = (Gobj::Material *)head();
        _head += sizeof(Gobj::Material) * counts.materials;
    }
    if (counts.meshes) {
        gobj->meshes = (Gobj::Mesh *)head();
        _head += sizeof(Gobj::Mesh) * counts.meshes;
        _head += sizeof(Gobj::MeshPrimative) * counts.meshPrimatives;
        _head += sizeof(Gobj::MeshAttribute) * counts.meshAttributes;
    }
    if (counts.nodes) {
        gobj->nodes = (Gobj::Node *)head();
        _head += sizeof(Gobj::Node) * counts.nodes;
    }
    if (counts.samplers) {
        gobj->samplers = (Gobj::Sampler *)head();
        _head += sizeof(Gobj::Sampler) * counts.samplers;
    }
    if (counts.scenes) {
        gobj->scenes = (Gobj::Scene *)head();
        _head += sizeof(Gobj::Scene) * counts.scenes;
    }
    if (counts.skins) {
        gobj->skins = (Gobj::Skin *)head();
        _head += sizeof(Gobj::Skin) * counts.skins;
    }
    if (counts.textures) {
        gobj->textures = (Gobj::Texture *)head();
        _head += sizeof(Gobj::Texture) * counts.textures;
    }

    buffers = head();
    _head += counts.buffersLen;

    assert(_head == dstSize);
    printl("_head == dstSize == %zu", _head);
}

// push crumb onto bread crumb stack
void GLTFLoader3::Loader::push(ObjType objType) {
    assert(depth < MaxDepth);
    crumbs[depth].objType = objType;
    // copy and consume key
    crumbs[depth].setKey(key);
    key[0] = '\0';
    ++depth;
}

void GLTFLoader3::Loader::pop() {
    --depth;
}

void GLTFLoader3::Loader::captureKey(char const * key) {
    snprintf(this->key, MaxKeyLen, "%s", key);
}

GLTFLoader3::Loader::Crumb & GLTFLoader3::Loader::crumbAt(int offset) {
    assert(depth && 1-(int)depth <= offset && offset <= 0);
    return crumbs[depth-1+offset];
}

size_t GLTFLoader3::Loader::handleData(byte_t * dst, char const * str, size_t strLength) {
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

bool GLTFLoader3::Loader::Null  ()           { return true; }
bool GLTFLoader3::Loader::Int   (int i)      { return true; }
bool GLTFLoader3::Loader::Int64 (int64_t  i) { return true; }
bool GLTFLoader3::Loader::Uint64(uint64_t u) { return true; }
bool GLTFLoader3::Loader::RawNumber(char const * str, size_t length, bool copy) { return true; }

bool GLTFLoader3::Loader::Bool (bool b) {
    if (C(-2,TYPE_ARR,"accessors")) {
        if (CC("normalized")) accessor()->normalized = b;
    }
    return true;
}

bool GLTFLoader3::Loader::Uint(unsigned u) {
    push(TYPE_INT);

    if (depth == 2 && CC("scene")) gobj->scene = u;

    else if (C(-2,TYPE_ARR,"accessors")) {
        if      (CC("bufferView"))    accessor()->bufferView = gobj->bufferViews + u;
        else if (CC("byteOffset"))    accessor()->byteOffset = u;
        else if (CC("componentType")) accessor()->componentType = (Gobj::Accessor::ComponentType)u;
        else if (CC("count"))         accessor()->count = u;
    }

    else if (C(-4,TYPE_ARR,"animations") && C(-2,TYPE_ARR,"channels")) {
        if (CC("sampler")) animationChannel()->sampler = gobj->samplers + u;
    }

    else if (C(-4,TYPE_ARR,"animations") && C(-2,TYPE_ARR,"samplers")) {
        if      (CC("input" )) animationSampler()->input  = gobj->accessors + u;
        else if (CC("output")) animationSampler()->output = gobj->accessors + u;
    }

    else if (C(-5,TYPE_ARR,"animations") && C(-3,TYPE_ARR,"channels") && C(-1,TYPE_OBJ,"target")) {
        if (CC("node")) animationChannel()->node = gobj->nodes + u;
    }

    else if (C(-2,TYPE_ARR,"buffers") && CC("byteLength")) buffer()->byteLength = u;

    else if (C(-2,TYPE_ARR,"bufferViews")) {
        if      (CC("buffer")) bufferView()->buffer = gobj->buffers + u;
        else if (CC("byteOffset")) bufferView()->byteOffset = u;
        else if (CC("byteLength")) bufferView()->byteLength = u;
        else if (CC("byteLength")) bufferView()->byteLength = u;
        else if (CC("byteStride")) bufferView()->byteStride = u;
        else if (CC("target")) bufferView()->target = (Gobj::BufferView::Target)u;

        // else if ()
    }

    pop();
    return true;
}

bool GLTFLoader3::Loader::Double(double d) {
    if (_floatPtr) {
        *_floatPtr = (float)d;
        // printl("writing %f to %p", d, _floatPtr);
        ++_floatPtr;
    }
    return true;
}

bool GLTFLoader3::Loader::String(char const * str, size_t length, bool copy) {
    push(TYPE_STR);

    if (C(-1,TYPE_OBJ,"asset")) {
        if      (CC("copyright" )) gobj->copyright  = strStack->writeStr(str, length);
        else if (CC("generator" )) gobj->generator  = strStack->writeStr(str, length);
        else if (CC("version"   )) gobj->version    = strStack->writeStr(str, length);
        else if (CC("minVersion")) gobj->minVersion = strStack->writeStr(str, length);
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

bool GLTFLoader3::Loader::Key(char const * str, size_t length, bool copy) {
    captureKey(str);
    return true;
}

bool GLTFLoader3::Loader::StartObject() {
    push(TYPE_OBJ);

    if (C(-3,TYPE_ARR,"animations") && C(-1,TYPE_ARR,"channels") && animation()->nChannels == 0)
        animation()->channels = animationChannel();
    if (C(-3,TYPE_ARR,"animations") && C(-1,TYPE_ARR,"samplers") && animation()->nSamplers == 0)
        animation()->samplers = animationSampler();

    return true;
}

bool GLTFLoader3::Loader::StartArray () {
    push(TYPE_ARR);
    if (C(-2,TYPE_ARR,"accessors")) {
        if      (CC("min")) _floatPtr = &accessor()->min[0];
        else if (CC("max")) _floatPtr = &accessor()->max[0];
    }
    return true;
}

bool GLTFLoader3::Loader::EndObject(size_t memberCount) {
    pop();

    // if      (CC("accessors" )) ++gobj->counts.accessors;
    // else if (CC("animations")) ++gobj->counts.animations;
    /*else*/ if (C(-2,TYPE_ARR,"animations") && C(0,TYPE_ARR,"channels")) {
        ++animation()->nChannels;
        // ++gobj->counts.animationChannels;
    }
    else if (C(-2,TYPE_ARR,"animations") && C(0,TYPE_ARR,"samplers")) {
        ++animation()->nSamplers;
        // ++gobj->counts.animationSamplers;
    }
    // else if (CC("buffers")) ++gobj->counts.buffers;
    // else if (CC("bufferViews")) ++gobj->counts.bufferViews;
    // else if (CC("cameras")) ++gobj->counts.cameras;

    return true;
}

bool GLTFLoader3::Loader::EndArray (size_t elementCount) {

    if (C(-2,TYPE_ARR,"accessors") && CC("min|max")) _floatPtr = nullptr;
    else if (C(-2,TYPE_ARR,"scenes") && CC("nodes")) {
        printl("found node children!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %zu", elementCount);
        printBreadcrumb();
    }

    pop();
    return true;
}

void GLTFLoader3::Loader::printBreadcrumb() const {
    if (depth < 1) return;
    for (int i = 0; i < depth; ++i) {
        print("%s(%s) > ", crumbs[i].objTypeStr(), crumbs[i].key);
    }
    printl();
}

Gobj::Accessor::Type GLTFLoader3::Loader::accessorTypeFromStr(char const * str) {
    if (strEqu(str, "SCALAR")) return Gobj::Accessor::TYPE_SCALAR;
    if (strEqu(str, "VEC2"  )) return Gobj::Accessor::TYPE_VEC2;
    if (strEqu(str, "VEC3"  )) return Gobj::Accessor::TYPE_VEC3;
    if (strEqu(str, "VEC4"  )) return Gobj::Accessor::TYPE_VEC4;
    if (strEqu(str, "MAT2"  )) return Gobj::Accessor::TYPE_MAT2;
    if (strEqu(str, "MAT3"  )) return Gobj::Accessor::TYPE_MAT3;
    if (strEqu(str, "MAT4"  )) return Gobj::Accessor::TYPE_MAT4;
    return Gobj::Accessor::TYPE_UNDEFINED;
}

Gobj::AnimationTarget GLTFLoader3::Loader::animationTargetFromStr(char const * str) {
    if (strEqu(str, "weights"    )) return Gobj::ANIM_TAR_WEIGHTS;
    if (strEqu(str, "translation")) return Gobj::ANIM_TAR_TRANSLATION;
    if (strEqu(str, "rotation"   )) return Gobj::ANIM_TAR_ROTATION;
    if (strEqu(str, "scale"      )) return Gobj::ANIM_TAR_SCALE;
    return Gobj::ANIM_TAR_UNDEFINED;
}

Gobj::AnimationSampler::Interpolation GLTFLoader3::Loader::interpolationFromStr(char const * str) {
    if (strEqu(str, "STEP"       )) return Gobj::AnimationSampler::INTERP_STEP;
    if (strEqu(str, "CUBICSPLINE")) return Gobj::AnimationSampler::INTERP_CUBICSPLINE;
    return Gobj::AnimationSampler::INTERP_LINEAR;
}

Gobj::Camera::Type GLTFLoader3::Loader::cameraTypeFromStr(char const * str) {
    if (strEqu(str, "orthographic")) return Gobj::Camera::TYPE_ORTHO;
    if (strEqu(str, "perspective" )) return Gobj::Camera::TYPE_PERSP;
    return Gobj::Camera::TYPE_PERSP;
}


byte_t * GLTFLoader3::Loader::head() const { return _dst + _head; }

Gobj::Accessor * GLTFLoader3::Loader::accessor() const {
    return gobj->accessors + gobj->counts.accessors;
}

Gobj::Animation * GLTFLoader3::Loader::animation() const {
    return gobj->animations + gobj->counts.animations;
}

Gobj::AnimationChannel * GLTFLoader3::Loader::animationChannel() const {
    return gobj->animationChannels + gobj->counts.animationChannels;
}

Gobj::AnimationSampler * GLTFLoader3::Loader::animationSampler() const {
    return gobj->animationSamplers + gobj->counts.animationSamplers;
}

Gobj::Buffer * GLTFLoader3::Loader::buffer() const {
    return gobj->buffers + gobj->counts.buffers;
}

Gobj::BufferView * GLTFLoader3::Loader::bufferView() const {
    return gobj->bufferViews + gobj->counts.bufferViews;
}

Gobj::Camera * GLTFLoader3::Loader::camera() const {
    return gobj->cameras + gobj->counts.cameras;
}

void GLTFLoader3::Loader::checkCounts() const {
    printl("Accessors  %2d == %2d", gobj->counts.accessors, counts.accessors);
    assert(gobj->counts.accessors == counts.accessors);
    printl("Animations %2d == %2d", gobj->counts.animations, counts.animations);
    assert(gobj->counts.animations == counts.animations);
    printl("AnimationChannels %2d == %2d", gobj->counts.animationChannels, counts.animationChannels);
    assert(gobj->counts.animationChannels == counts.animationChannels);
    printl("AnimationSamplers %2d == %2d", gobj->counts.animationSamplers, counts.animationSamplers);
    assert(gobj->counts.animationSamplers == counts.animationSamplers);
}





// =============================================================================
// =============================================================================
// =============================================================================



Gobj::Counts GLTFLoader3::calcDataSize(char const * jsonStr) {
    GLTFLoader3::SizeFinder scanner;
    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, scanner);
    return scanner.counter();
}

bool GLTFLoader3::load(Gobj * g, size_t dstSize, Gobj::Counts const & counts, char const * jsonStr) {
    Loader loader{(byte_t *)g, dstSize, counts};
    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, loader);
    return true;
}

void GLTFLoader3::prettyStr(char const * jsonStr) {

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, writer);
    printf("%s\n", sb.GetString());
    // return mm.frameFormatStr("%s", );
}
