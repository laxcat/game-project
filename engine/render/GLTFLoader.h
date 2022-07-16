#pragma once
#include <mutex>
#include <glm/mat4x4.hpp>
#include "Renderable.h"


class GLTFLoader {
public:
    std::mutex loadingMutex;

    void load(char const * filename, Renderable & renderable);

private:
    void traverseNodes(
        Renderable & renderable, 
        tinygltf::Model const & model, 
        glm::mat4 mat, 
        std::vector<int> const & nodes, 
        int level);
    
    void processPrimitive(
        Renderable & renderable, 
        tinygltf::Model const & model, 
        tinygltf::Primitive const & primitive,
        glm::mat4 mat
    );

    void printModel(bool shouldPrint, tinygltf::Model const & model);

};
