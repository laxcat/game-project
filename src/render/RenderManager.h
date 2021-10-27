#pragma once
#include <vector>
#include "Renderable.h"


class RenderManager {
public:

    std::vector<Renderable *> renderables;

    void loadGLTF(char const * filename) {
        using namespace tinygltf;

        Model model;
        TinyGLTF loader;
        std::string err;
        std::string warn;

        if (loader.LoadBinaryFromFile(&model, &err, &warn, filename)) {
            printf("successfully loaded %s\n", filename);
        }
        else {
            printf("failed to load %s\n", filename);
        }

        Scene const & scene = model.scenes[model.defaultScene];
        traverseGLTFNodes(model, scene.nodes, 0);
    }

    void add(Renderable * renderable) {
        renderables.push_back(renderable);
    }

    void draw() {
        for (auto * renderable : renderables) {
            renderable->draw();
        }
    }

    void shutdown() {
        for (auto * renderable : renderables) {
            renderable->shutdown();
        }
    }

private:
    void traverseGLTFNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level) {
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
};
