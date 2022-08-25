#include <glm/gtc/quaternion.hpp>
#include "GLTFLoader.h"
#include "../MrManager.h"
#include "../common/utils.h"
#include "../common/string_utils.h"


static bool constexpr ShowGLTFLoadDbg = false;


void GLTFLoader::load(char const * filename, Renderable & renderable) {
    // loadState SHOULD be the only thing we need to lock 🤞
    // start loading
    loadingMutex.lock();
    renderable.loadState = Renderable::LoadState::Loading;
    loadingMutex.unlock();

    // setup vars
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // try to load as binary
    bool loaded = false;
    try {
        printc(ShowGLTFLoadDbg, "Trying to load as binary: %s\n", relPath(filename));
        // populate member variable `model`
        loaded = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    }
    catch(...) {}

    // binary load failed, try text
    if (!loaded) {
        printc(ShowGLTFLoadDbg, "Failed to load as binary: %s\n", relPath(filename));
        try {
            printc(ShowGLTFLoadDbg, "Trying to load as text: %s\n", relPath(filename));
            // populate member variable `model`
            loaded = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
        }
        catch(...) {}
    }

    // both loads failed
    if (!loaded) {
        printc(ShowGLTFLoadDbg, "Failed to load as text: %s\n", relPath(filename));
        loadingMutex.lock();
        renderable.loadState = Renderable::LoadState::FailedToLoad;
        loadingMutex.unlock();
        return;
    }

    // print model
    printModel(ShowGLTFLoadDbg, model);

    // copy the buffer over to our memory
    // (as it apparently does not retain once loader is out of scope)
    if (model.buffers.size() > 1) {
        print("WARNING, UNSUPPORTED: more than one buffer found in gltf.\n");
    }
    renderable.bufferSize = model.buffers[0].data.size();
    renderable.buffer = (byte_t *)malloc(renderable.bufferSize);
    memcpy(renderable.buffer, model.buffers[0].data.data(), renderable.bufferSize);

    // setup images
    for (size_t i = 0; i < model.images.size(); ++i) {
        Image img;
        img.width = model.images[i].width;
        img.height = model.images[i].height;
        img.comp = model.images[i].component;
        img.dataSize = model.images[i].image.size();
        img.data = (byte_t *)malloc(img.dataSize);
        memcpy(img.data, model.images[i].image.data(), img.dataSize);
        renderable.images.push_back(img);
    }

    // setup textures
    for (size_t i = 0; i < model.textures.size(); ++i) {

        auto iid = model.textures[i].source;
        Image & img = renderable.images[iid];

        auto handle = bgfx::createTexture2D(
            uint16_t(img.width), 
            uint16_t(img.height), 
            false, 
            1, 
            bgfx::TextureFormat::RGBA8, 
            0, 
            img.ref()
        );

        renderable.textures.push_back(handle);
    }

    // setup materials
    size_t matCount = model.materials.size();
    for (int i = 0; i < matCount; ++i) {
        auto & material = model.materials[i];
        // print("MATERIAL %s\n", material.name.c_str());
        Material mat;
        setName(mat.name, material.name.c_str());
        // mat.name[0] = '\0', strncat(mat.name, material.name.c_str(), 31);
        mat.baseColor = glmVec4(material.pbrMetallicRoughness.baseColorFactor.data());
        renderable.materials.push_back(mat);
    }

    // rotate Y axis 180° by default
    renderable.adjRotAxes[0] = false;
    renderable.adjRotAxes[1] = true;
    renderable.adjRotAxes[2] = false;
    renderable.updateAdjRot();

    glm::mat4 mat{1.f};

    // traverse all nodes to find our primatives and meshes
    renderable.meshes.clear();
    traverseNodes(renderable, model, mat, model.scenes[model.defaultScene].nodes, 0);

    // move all alpha meshes to end of regular meshes
    renderable.meshes.insert(
        renderable.meshes.end(),
        renderable.meshesWithAlpha.begin(),
        renderable.meshesWithAlpha.end()
    );
    renderable.meshesWithAlpha.clear();

    loadingMutex.lock();
    renderable.loadState = Renderable::LoadState::WaitingToFinalize;
    loadingMutex.unlock();
}

void GLTFLoader::traverseNodes(
    Renderable & renderable, 
    tinygltf::Model const & model, 
    glm::mat4 parentMat,
    std::vector<int> const & nodes, 
    int level
    ) {
    size_t nodeCount = nodes.size();
    for (size_t i = 0; i < nodeCount; ++i) {
        tinygltf::Node const & node = model.nodes[nodes[i]];

        glm::mat4 nodeMat{1.f};
        glm::mat4 mat;

        // if matrix exists use that
        if (node.matrix.size() == 16) {
            nodeMat = glmMat4(node.matrix.data());

            // printf("Node %s MAT:\n", node.name.size()?node.name.c_str():"");
            // print16f((float *)&nodeMat);

            mat = parentMat * nodeMat;
        }
        // otherwise use TRS
        // if none of TRS exists, nothing happens to mat
        else {
            glm::mat4 tMat{1.f};
            glm::mat4 rMat{1.f};
            glm::mat4 sMat{1.f};

            // printf("Node %s TRS:\n", node.name.size()?node.name.c_str():"");

            // TODO: sometimes the matrix might be used? (node.matrix)
            // handle transforms
            if (node.translation.size() == 3) {
                tMat = glm::translate(tMat,
                    glm::vec3{
                        (float)node.translation[0], 
                        (float)node.translation[1], 
                        (float)node.translation[2]
                    }
                );
                // printf("T:\n");
                // print16f((float *)&tMat);
            }
            if (node.rotation.size() == 4) {
                rMat = glm::mat4_cast(
                    // the `glm::quat` constructor takes:
                    // w, x, y, z! w first!!!
                    // tiny_gltf follows gltf which is xyzw, so make note of subindices below.
                    // https://stackoverflow.com/a/49416643/560384
                    //
                    // This `quat` constructor is not changed by GLM_FORCE_QUAT_DATA_WXYZ, 
                    // which only seems to change the data order. `w` is always first here.
                    // ¯\_(ツ)_/¯
                    //
                    glm::quat{
                        (float)node.rotation[3], // w
                        (float)node.rotation[0], // x
                        (float)node.rotation[1], // y
                        (float)node.rotation[2]  // z
                    }
                );
                // printf("R:\n");
                // print16f((float *)&rMat);
            }
            if (node.scale.size() == 3) {
                sMat = glm::scale(sMat, 
                    glm::vec3{
                        (float)node.scale[0], 
                        (float)node.scale[1], 
                        (float)node.scale[2]
                    }
                );
                // printf("S:\n");
                // print16f(float *)&sMat);
            }
            nodeMat = tMat * rMat * sMat;

            // printf("combined:\n");
            // print16f((float *)&nodeMat);
            // printf("\n");

            mat = parentMat * nodeMat;
        }

        if (node.mesh >= 0) {
            tinygltf::Mesh const & mesh = model.meshes[node.mesh];
            for (size_t p = 0; p < mesh.primitives.size(); ++p) {
                processPrimitive(renderable, model, mesh.primitives[p], mat);
            }
        }
        traverseNodes(renderable, model, mat, node.children, level+1);
    }
}

void GLTFLoader::processPrimitive(
    Renderable & renderable, 
    tinygltf::Model const & model, 
    tinygltf::Primitive const & primitive,
    glm::mat4 mat
    ) {
    // fprintf(stderr, "PRIMITIVE!!!!!! %p\n", &primitive);

    // make one "Mesh" for every primitive
    Mesh mesh;
    mesh.renderableKey = renderable.key;

    // will sort into seperate vector, then merge into sorted vector afterwards
    bool hasAlpha = false;

    // copy matrix
    mesh.model = mat;

    // check mode
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        print("WARNING UNRECOGNIZED MODE!: %d\n", primitive.mode);
    }

    // setup index buffer info
    tinygltf::Accessor const & indexAcc = model.accessors[primitive.indices];
    if (indexAcc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        print("WARNING UNRECOGNIZED INDEX COMPONENT TYPE!: %d\n", indexAcc.componentType);
    }
    tinygltf::BufferView const & indexBV = model.bufferViews[indexAcc.bufferView];
    // byte_t const * indexData = model.buffers[indexBV.buffer].data.data();
    auto indices_temp = (unsigned short *)(renderable.buffer + indexAcc.byteOffset + indexBV.byteOffset);
    // fprintf(stderr, "TRIANGLES!!!!\n");
    // for (size_t i = 0; i < indexAcc.count / 3; ++i) {
    //     unsigned short * d = indices_temp + i * 3;
    //     fprintf(stderr, "(%d, %d, %d)\n", d[0], d[1], d[2]);
    // }
    auto indexRef = bgfx::makeRef(indices_temp, indexAcc.count * 2);
    mesh.ibuf = bgfx::createIndexBuffer(indexRef);

    // collect all the relevent data from Attrributes, Accessors, BufferViews, Buffers
    struct AttribInfo {
        bgfx::Attrib::Enum type;
        std::string typeString;
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
            (attribPair.first == "COLOR_0")     ? bgfx::Attrib::Color0      :
            (attribPair.first == "WEIGHTS_0")   ? bgfx::Attrib::Weight      :
            (attribPair.first == "TEXCOORD_0")  ? bgfx::Attrib::TexCoord0   :
            (attribPair.first == "TEXCOORD_1")  ? bgfx::Attrib::TexCoord1   :
            (attribPair.first == "TEXCOORD_2")  ? bgfx::Attrib::TexCoord2   :
            (attribPair.first == "TEXCOORD_3")  ? bgfx::Attrib::TexCoord3   :
            bgfx::Attrib::Count;
        if (ai.type == bgfx::Attrib::Count) {
            print("WARNING UNRECOGNIZED ATTRIBUTE!: %s\n", attribPair.first.c_str());
        }
        ai.typeString = attribPair.first;

        // only support floats for now. reasonable restriction for vbuffs?
        if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            print("WARNING UNRECOGNIZED VERTEX COMPONENT TYPE!: %d\n", acc.componentType);
        }
        ai.compType = bgfx::AttribType::Float;

        ai.compCount = 
            (acc.type == TINYGLTF_TYPE_VEC2) ? 2 :
            (acc.type == TINYGLTF_TYPE_VEC3) ? 3 :
            (acc.type == TINYGLTF_TYPE_VEC4) ? 4 :
            0;
        if (ai.compCount == 0) {
            print("WARNING UNRECOGNIZED COMPONENT COUNT!: %d\n", acc.type);
        }

        ai.bufViewId = acc.bufferView;
        ai.stride = bv.byteStride;
        ai.bufId = bv.buffer;
        ai.offset = bv.byteOffset + acc.byteOffset;
        ai.count = acc.count;
        ai.byteSize = acc.count * ai.compCount * 4; // (always float; always 4)

        info.push_back(ai);
    }

    // set base texture
    int mid, tid, iid;
    if ((mid = primitive.material) >= 0) {
        auto & m = model.materials[mid];
        auto & pbr = m.pbrMetallicRoughness;
        if ((tid = pbr.baseColorTexture.index) >= 0 &&
            (iid = model.textures[tid].source) >= 0
            ) {
            mesh.images.color = tid;
        }
        mesh.materialId = mid;

        if (m.alphaMode == "BLEND") {
            // fprintf(stderr, "FOUND ONE!!!!!!!!!!!!!!!!!!! %s %f\n", m.name.c_str(), mesh.images.baseColor[3]);
            hasAlpha = true;
        }
    }
    // printf("mesh (%s) images color: %d\n", 
    //     model.materials[mid].name.size()?model.materials[mid].name.c_str():"",
    //     mesh.images.color);

    // printf("--------------------------------------------------------------------------------\n");
    // printf("ATTRIBUTES (before sort):\n");
    // for (size_t i = 0; i < info.size(); ++i) {
    //     auto & ai = info[i];
    //     printf(
    //         "ATTRIB INFO index:%zu type:%s (%d), bufViewId:%d, stride:%d, "
    //         "bufId:%d, offset:%d, count:%d, compCount: %d, size:%d\n", 
    //         i, ai.typeString.c_str(), ai.type, ai.bufViewId, ai.stride, 
    //         ai.bufId, ai.offset, ai.count, ai.compCount, ai.byteSize
    //     );
    // }
    // printf("--------------------------------------------------------------------------------\n");


    // if no overlap, create one vbuf for each accessor
    if (1) {
        // // sort by attribute type, then buffer, then bufferview, then offset (totaled from bv and accessor)
        // std::sort(info.begin(), info.end(), [](auto a, auto b){ 
        //     return 
        //         (a.type != b.type)              ? a.type < b.type :
        //         (a.bufId != b.bufId)            ? a.bufId < b.bufId :
        //         (a.bufViewId != b.bufViewId)    ? a.bufViewId < b.bufViewId :
        //         a.offset < b.offset;
        // });

        // for each attrib
        for (size_t i = 0; i < info.size(); ++i) {
            auto & ai = info[i];
            // byte_t const * data = model.buffers[ai.bufId].data.data();

            // printf("ATTRIB INFO index:%zu type:%s (%d), bufViewId:%d, stride:%d, bufId:%d, offset:%d, count:%d, compCount: %d, size:%d\n", 
            //     i, ai.typeString.c_str(), ai.type, ai.bufViewId, ai.stride, ai.bufId, ai.offset, ai.count, ai.compCount, ai.byteSize);

            // create layout
            bgfx::VertexLayout layout;
            layout.begin();
            layout.add(ai.type, ai.compCount, ai.compType);
            layout.end();

            auto ref = bgfx::makeRef(renderable.buffer + ai.offset, ai.byteSize);
            auto vbuf = bgfx::createVertexBuffer(ref, layout);

            // add a vbuf
            mesh.vbufs.push_back(vbuf);

            // fprintf(stderr, "VERTICES!!!!\n");
            // for (size_t i = 0; i < ai.count; ++i) {
            //     float const * d = (float *)(r->buffer + ai.offset + i * 12);
            //     fprintf(stderr, "(%f, %f, %f)\n", d[0], d[1], d[2]);
            // }

        }
    }

    // TODO: handle interleaved buffers
    // overlap (interleaved) combine multiple accessors into one buffer
    if (0) {

    }

    if (hasAlpha){
        renderable.meshesWithAlpha.push_back(mesh);
    }
    else {
        renderable.meshes.push_back(mesh);
    }
}


void GLTFLoader::printModel(bool shouldPrint, tinygltf::Model const & model) {
    if (!shouldPrint) return;

    char indent[128];
    char indentNext[128];
    size_t totalTriCount = 0;
    
    // recursive function, takes list of nodes and level
    std::function<void(std::vector<int> const &, int)> printNodes;
    printNodes = [&](std::vector<int> const & nodes, int level){
        size_t nodeCount = nodes.size();
        for (size_t i = 0; i < nodeCount; ++i) {
            tinygltf::Node const & node = model.nodes[nodes[i]];

            memset(indent, ' ', 128);
            indent[level*4] = '\0';
            memset(indentNext, ' ', 128);
            indentNext[(level+1)*4] = '\0';

            // print node
            print("%sNode(%s, child count: %zu, mesh: %d (%s))\n", 
                indent, 
                node.name.c_str(), 
                node.children.size(), 
                node.mesh,
                (node.mesh == -1) ? "" : model.meshes[node.mesh].name.c_str()
            );

            // print matrix if not identity
            if (node.translation.size()) {
                print("%sTRANSLATION: ", indentNext);
                for (auto v : node.translation) {
                    print(" %0.4f", v);
                }
                print("\n");
            }
            if (node.rotation.size()) {
                print("%sROTATION: ", indentNext);
                for (auto v : node.rotation) {
                    print(" %0.4f", v);
                }
                print("\n");
            }
            if (node.scale.size()) {
                print("%sSCALE: ", indentNext);
                for (auto v : node.scale) {
                    print(" %0.4f", v);
                }
                print("\n");
            }
            if (node.matrix.size()) {
                print("%sMATRIX: \n", indentNext);
                glm::mat4 m = glmMat4(node.matrix.data());
                print16f((float *)&m, indentNext);
            }
    
            // print primitives if any
            if (node.mesh >= 0) {
                tinygltf::Mesh const & mesh = model.meshes[node.mesh];
                for (size_t p = 0; p < mesh.primitives.size(); ++p) {
                    size_t triCount = model.accessors[mesh.primitives[p].indices].count / 3;
                    print("%sPrimitive(mode: %d, indices: %d, tri-count: %zu, material: %d)\n", 
                        indentNext, 
                        mesh.primitives[p].mode,
                        mesh.primitives[p].indices,
                        triCount,
                        mesh.primitives[p].material
                    );
                    totalTriCount += triCount;
                }
            }
    
            printNodes(node.children, level+1);
        }
    };
    
    print("--------------------------------------------------------------------------------\n");

    // loop through scenes
    for (size_t i = 0; i < model.scenes.size(); ++i) {
        print("Scene %zu (%s)%s\n", 
            i, 
            model.scenes[i].name.c_str(), 
            (i == model.defaultScene) ? ", DEFAULT" : ""
        );
        printNodes(model.scenes[i].nodes, 1);
        print("Scene %zu triangle count: %zu\n", i, totalTriCount);
    }

    // loop through materials
    for (size_t i = 0; i < model.materials.size(); ++i) {
        auto & m = model.materials[i];
        auto & pbr = m.pbrMetallicRoughness;
        print("Material(%s, ti index: %d, ti texCoord: %d, bcf: %02x%02x%02x%02x, metal/rough: %0.3f/%0.3f)\n", 
            m.name.size() ? m.name.c_str() : "",
            pbr.baseColorTexture.index,
            pbr.baseColorTexture.texCoord,
            int(pbr.baseColorFactor[0] * 255),
            int(pbr.baseColorFactor[1] * 255),
            int(pbr.baseColorFactor[2] * 255),
            int(pbr.baseColorFactor[3] * 255),
            pbr.metallicFactor, 
            pbr.roughnessFactor
        );
    }

    // loop through textures
    for (size_t i = 0; i < model.textures.size(); ++i) {
        auto & t = model.textures[i];
        print("Texture(%s, sampler: %d, source: %d)\n", 
            t.name.size() ? t.name.c_str() : "",
            t.sampler,
            t.source
        );
    }

    // loop through images
    for (size_t i = 0; i < model.images.size(); ++i) {
        print("Image(W×H: %d×%d,%d, bits-per-channel: %d, data size: %zu, \"as is\": %s)\n", 
            model.images[i].width,
            model.images[i].height,
            model.images[i].component,
            model.images[i].bits,
            model.images[i].image.size(),
            model.images[i].as_is ? "true":"false"
        );
    }

    print("--------------------------------------------------------------------------------\n");
}
