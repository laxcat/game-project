#pragma once
#include <unordered_map>
#include "Renderable.h"


class RenderableManager {
public:

    void traverseNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level) {
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
            traverseNodes(model, node.children, level+1);
        }
    }

    void loadGLTF(char const * filename) {
        using namespace tinygltf;

        Model model;
        TinyGLTF loader;
        std::string err;
        std::string warn;

        char const * fn = "MetalRoughSpheres.glb";
        if (loader.LoadBinaryFromFile(&model, &err, &warn, fn)) {
            printf("successfully loaded %s\n", fn);
        }
        else {
            printf("failed to load %s\n", fn);
        }

        Scene const & scene = model.scenes[model.defaultScene];
        traverseNodes(model, scene.nodes, 0);
    }

    std::unordered_map<size_t, Renderable> renderables;
};
