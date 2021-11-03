#include "GLTFRenderable.h"


void GLTFRenderable::load(char const * filename) {
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
    traverseNodes(model, scene.nodes, 0);
}

void GLTFRenderable::traverseNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level) {
    using namespace tinygltf;

    size_t nodeCount = nodes.size();
    char indent[32];
    memset(indent, ' ', 32);
    indent[level*4] = '\0';
    for (size_t i = 0; i < nodeCount; ++i) {
        Node const & node = model.nodes[nodes[i]];
        if (node.mesh > 0) {
            Mesh const & mesh = model.meshes[node.mesh];
            for (size_t p = 0; p < mesh.primitives.size(); ++p) {
                processPrimitive(model, mesh.primitives[p]);
            }
        }
        printf("%snode(%s, child count: %zu, mesh: %d (%s))\n", 
            indent, 
            node.name.c_str(), 
            node.children.size(), 
            node.mesh,
            model.meshes[node.mesh].name.c_str()
        );
        traverseNodes(model, node.children, level+1);
    }
}

void GLTFRenderable::processPrimitive(tinygltf::Model const & model, tinygltf::Primitive const & primitive) {
    using namespace tinygltf;

    // map all buffers and buffer views
    // associate all accessors with buffer views
    // create one vertex buffer for every 


    // Mesh const & mesh = model.meshes[meshIndex];

    // // bgfx::VertexLayout layout;
    // // layout.begin();

    // // for each buffer view
    // for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    //     BufferView const & bufferView = model.bufferViews[i];

    //     // bgfx::VertexLayout layout;
    //     // layout.begin();

    //     // for each accessor
    //     for (size_t a = 0; a < model.accessors.size(); ++a) {
    //         Accessor const & accessor = model.accessors[a];
    //         if (accessor.bufferView != i) continue;

    //         printf("ACCESSOR %zu, bv: %d, type: 0x%04x, ctype: 0x%04x\n", a, accessor.bufferView, accessor.type, accessor.componentType);
    //     }

    //     switch (bufferView.target) {
    //         case GLArrayBuffer: {
    //             // initVerts(bufferView.byteLength);
    //             // vbh = bgfx::createVertexBuffer(vertsRef(), PosColorVertex::layout);
    //         }
    //         case GLElementArrayBuffer: {

    //         }
    //         default: {

    //         }
    //     }

    //     printf("buffer view %zu %s, 0x%x (%s)\n", 
    //         i, 
    //         bufferView.name.c_str(), 
    //         bufferView.target, 
    //         glString(bufferView.target)
    //     );
    // }
}
