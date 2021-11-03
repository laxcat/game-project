#pragma once
#include "Renderable.h"


class GLTFRenderable : public Renderable {
public:
    void load(char const * filename);
    void traverseBuffers(tinygltf::Model const & model);
    void traverseNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level);
};