#include "GLTFLoader3.h"
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>


/*

A rapidjson Handler that gathers the necessary info from the GLTF json to
calculate byte-size of final game object.

*/

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
    objCountConds.push(0, Crumb::ARR, "accessors", [this](void * ud){ cntr.accessors = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "animations", [this](void * ud){ cntr.animations = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "buffers", [this](void * ud){ cntr.buffers = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "bufferViews", [this](void * ud){ cntr.bufferViews = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "cameras", [this](void * ud){ cntr.cameras = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "images", [this](void * ud){ cntr.images = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "materials", [this](void * ud){ cntr.materials = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "meshes", [this](void * ud){ cntr.meshes = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "nodes", [this](void * ud){ cntr.nodes = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "samplers", [this](void * ud){ cntr.samplers = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "scenes", [this](void * ud){ cntr.scenes = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "skins", [this](void * ud){ cntr.skins = (size_t)ud; });
    objCountConds.push(0, Crumb::ARR, "textures", [this](void * ud){ cntr.textures = (size_t)ud; });

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
    byteCountConds.push(-2, Crumb::ARR, "buffers");\


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

Gobj::Counts GLTFLoader3::calcDataSize(char const * jsonStr) {
    GLTFLoader3::SizeFinder scanner;
    rapidjson::Reader reader;
    auto fs = rapidjson::StringStream(jsonStr);
    reader.Parse(fs, scanner);
    return scanner.counter();
}

bool GLTFLoader3::load(Gobj * g, char const * jsonStr) {
    return true;
}
