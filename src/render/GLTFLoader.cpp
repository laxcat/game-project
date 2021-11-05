#include "GLTFLoader.h"
#include "../MrManager.h"


void GLTFLoader::load(char const * filename, size_t renderable) {
    using namespace tinygltf;

    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    // populate member variable `model`
    if (loader.LoadBinaryFromFile(&model, &err, &warn, filename)) {
        printf("successfully loaded %s\n", filename);
    }
    else {
        printf("failed to load %s\n", filename);
    }

    Scene const & scene = model.scenes[model.defaultScene];
    traverseNodes(renderable, model, scene.nodes, 0);
}

void GLTFLoader::traverseNodes(size_t renderable, tinygltf::Model const & model, std::vector<int> const & nodes, int level) {
    using namespace tinygltf;

    size_t nodeCount = nodes.size();
    char indent[32];
    memset(indent, ' ', 32);
    indent[level*4] = '\0';
    for (size_t i = 0; i < nodeCount; ++i) {
        Node const & node = model.nodes[nodes[i]];
        printf("%snode(%s, child count: %zu, mesh: %d (%s))\n", 
            indent, 
            node.name.c_str(), 
            node.children.size(), 
            node.mesh,
            model.meshes[node.mesh].name.c_str()
        );
        if (node.mesh >= 0) {
            tinygltf::Mesh const & mesh = model.meshes[node.mesh];
            for (size_t p = 0; p < mesh.primitives.size(); ++p) {
                processPrimitive(renderable, model, mesh.primitives[p]);
            }
        }
        traverseNodes(renderable, model, node.children, level+1);
    }
}

void GLTFLoader::processPrimitive(size_t renderable, tinygltf::Model const & model, tinygltf::Primitive const & primitive) {
    // make one "Mesh" for every primitive
    auto & meshes = mm.rendMan.at(renderable)->meshes;
    meshes.push_back({});
    Mesh & mesh = meshes.back();

    // check mode
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        fprintf(stderr, "WARNING UNRECOGNIZED MODE!: %d\n", primitive.mode);
    }

    // setup index buffer info
    tinygltf::Accessor const & indexAcc = model.accessors[primitive.indices];
    tinygltf::BufferView const & indexBV = model.bufferViews[indexAcc.bufferView];
    byte_t const * indexData = model.buffers[indexBV.buffer].data.data();
    mesh.ibuf = bgfx::createIndexBuffer(bgfx::makeRef(indexData, indexAcc.count * 2));
    if (indexAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        fprintf(stderr, "WARNING UNRECOGNIZED INDEX COMPONENT TYPE!: %d\n", indexAcc.componentType);
    }

    // collect all the relevent data from Attrributes, Accessors, BufferViews, Buffers
    struct AttribInfo {
        bgfx::Attrib::Enum type;
        bgfx::AttribType::Enum compType;
        int compCount;
        int bufViewId;
        int stride;
        int bufId;
        int offset;
        int count;
        int byteSize;
    };
    std::vector<AttribInfo> info;

    // for each attribute...
    for (auto attribPair : primitive.attributes) {
        tinygltf::Accessor const & acc = model.accessors[attribPair.second];
        tinygltf::BufferView const & bv = model.bufferViews[acc.bufferView];

        AttribInfo ai;

        ai.type = 
            (attribPair.first == "POSITION")    ? bgfx::Attrib::Position    :
            (attribPair.first == "NORMAL")      ? bgfx::Attrib::Normal      :
            (attribPair.first == "TANGENT")     ? bgfx::Attrib::Tangent     :
            (attribPair.first == "TEXCOORD_0")  ? bgfx::Attrib::TexCoord0   :
            (attribPair.first == "TEXCOORD_1")  ? bgfx::Attrib::TexCoord1   :
            (attribPair.first == "TEXCOORD_2")  ? bgfx::Attrib::TexCoord2   :
            (attribPair.first == "TEXCOORD_3")  ? bgfx::Attrib::TexCoord3   :
            (attribPair.first == "COLOR_0")     ? bgfx::Attrib::Color0      :
            (attribPair.first == "WEIGHTS_0")   ? bgfx::Attrib::Weight      :
            bgfx::Attrib::Count;
        if (ai.type == bgfx::Attrib::Count) {
            fprintf(stderr, "WARNING UNRECOGNIZED ATTRIBUTE!: %s\n", attribPair.first.c_str());
        }

        // only support floats for now. reasonable restriction for vbuffs?
        if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            fprintf(stderr, "WARNING UNRECOGNIZED VERTEX COMPONENT TYPE!: %d\n", acc.componentType);
        }
        ai.compType = bgfx::AttribType::Float;

        ai.compCount = 
            (acc.type == TINYGLTF_TYPE_VEC2) ? 2 :
            (acc.type == TINYGLTF_TYPE_VEC3) ? 3 :
            (acc.type == TINYGLTF_TYPE_VEC4) ? 4 :
            0;
        if (ai.compCount == 0) {
            fprintf(stderr, "WARNING UNRECOGNIZED COMPONENT COUNT!: %d\n", acc.type);
        }

        ai.bufViewId = acc.bufferView;
        ai.stride = bv.byteStride;
        ai.bufId = bv.buffer;
        ai.offset = bv.byteOffset + acc.byteOffset;
        ai.count = acc.count;
        ai.byteSize = acc.count * ai.compCount * 4; // (always float; always 4)

        info.push_back(ai);
    }

    // sort by buffer, then bufferview, then offset (totaled from bv and accessor)
    std::sort(info.begin(), info.end(), [](auto a, auto b){ 
        return 
            (a.bufId != b.bufId)            ? a.bufId < b.bufId :
            (a.bufViewId != b.bufViewId)    ? a.bufViewId < b.bufViewId :
            a.offset < b.offset;
    });

    // if no overlap, create one vbuf for each accessor
    if (1) {

        // for each attrib
        for (size_t i = 0; i < info.size(); ++i) {
            auto & ai = info[i];
            byte_t const * data = model.buffers[ai.bufId].data.data();

            printf("ATTRIB INFO index:%zu type:%d, bufViewId:%d, stride:%d, bufId:%d, offset:%d, count:%d, compCount: %d, size:%d\n", 
                i, ai.type, ai.bufViewId, ai.stride, ai.bufId, ai.offset, ai.count, ai.compCount, ai.byteSize);

            // create layout
            bgfx::VertexLayout layout;
            layout.begin();
            layout.add(ai.type, ai.compCount, ai.compType);
            layout.end();

            // copy vert data
            mesh.initVerts(ai.count, layout.getStride());
            memcpy(mesh.verts, data + ai.offset, ai.byteSize);

            // add a vbuf
            mesh.vbufs.push_back(bgfx::createVertexBuffer(mesh.vertsRef(), layout));
        }
    }

    // overlap (interleaved) combine multiple accessors into one buffer
    if (0) {

    }
}
