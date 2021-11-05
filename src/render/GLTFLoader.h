#pragma once
#include "Renderable.h"


class GLTFLoader {
public:
    void load(char const * filename, size_t renderable);

private:
    void traverseNodes(size_t renderable, tinygltf::Model const & model, std::vector<int> const & nodes, int level);
    void processPrimitive(size_t renderable, tinygltf::Model const & model, tinygltf::Primitive const & primitive);
};
