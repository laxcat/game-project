#pragma once
#include "Renderable.h"


class GLTFRenderable : public Renderable {
public:
    void load(char const * filename);
    void traverseNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level);
    void processPrimitive(tinygltf::Model const & model, tinygltf::Primitive const & primitive);
};
