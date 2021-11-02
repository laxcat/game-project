#include "Renderable.h"


void Renderable::loadGLTF(char const * filename) {
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
    traverseGLTFNodes(model, scene.nodes, 0);
}


void Renderable::traverseGLTFNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level) {
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
        traverseGLTFNodes(model, node.children, level+1);
    }
}
