#pragma once
#include <mutex>
#include <glm/mat4x4.hpp>
#include <tiny_gltf.h>

class Renderable;

class GLTFLoader {
public:
    std::mutex loadingMutex;

    void load(Renderable * renderable);

private:
    void traverseNodes(
        Renderable * r,
        tinygltf::Model const & model, 
        glm::mat4 mat, 
        std::vector<int> const & nodes, 
        int level);
    
    void processPrimitive(
        Renderable * r,
        tinygltf::Model const & model, 
        tinygltf::Primitive const & primitive,
        glm::mat4 mat
    );

    void printModel(bool shouldPrint, tinygltf::Model const & model);

};
