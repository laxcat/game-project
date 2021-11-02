#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Renderable.h"


class RenderManager {
public:
    // 
    // MANAGER LIFECYCLE
    //

    void draw() {
        for (auto i : renderables) {
            at(i)->draw();
        }
    }

    void shutdown() {
        Renderable * r;
        for (size_t i = 0; i < poolSize; ++i) {
            r = at(i);
            if (r->active) r->shutdown();
            r->active = false;
        }
    }


    // 
    // RENDERABLE LIFECYCLE
    //

    template<typename T>
    size_t create(char const * key = "") {
        static_assert(std::is_base_of<Renderable, T>::value, "T must be (or subclass of) Renderable");
        if (poolNextFree >= poolSize) {
            fprintf(stderr, "WARNING: did not create renderable. Pool full.");
            return SIZE_MAX;
        }
        if (strcmp(key, "") != 0 && keyExists(key)) {
            fprintf(stderr, "WARNING: did not create renderable. Key already in use.");
            return SIZE_MAX;
        }

        size_t index = poolNextFree;
        byte_t * ptr = (byte_t *)&pool[0] + index * sizeof(Renderable);
        fprintf(stderr, "CREATING AT %zu (%p)\n", index, ptr);
        new (ptr) T();
        T * renderable = at<T>(index);
        renderable->init();
        renderable->active = true;
        renderables.push_back(index);
        if (strcmp(key, "") != 0) {
            renderable->key = key;
            keys.emplace(key, index);
        }

        setPoolNextFree(index+1);
        return index;
    }
    size_t create(char const * key = "") { return create<Renderable>(key); }

    template<typename T>
    T * at(int index) {
        assert(index < poolSize && "Out of bounds.");
        return static_cast<T *>(&pool[index]);
    }

    Renderable * at(int index) {
        return at<Renderable>(index);
    }

    template<typename T>
    T * at(char const * key) {
        return at<T>(keys.at(key));
    }

    Renderable * at(char const * key) {
        return at(keys.at(key));
    }

    void destroy(size_t index) {
        auto * r = at(index);
        r->shutdown();
        r->active = false;
        for(auto i = renderables.begin(); i != renderables.end(); ++i) {
            if (*i == index) {
                renderables.erase(i);
                break;
            }
        }
        if (keys.at(r->key)) keys.erase(r->key);

        // we know this slot is free, but it shouldn't be the new free slot
        // if there is already a lower one
        if (poolNextFree > index)
            poolNextFree = index;
    }

    void destroy(char const * key) {
        destroy(keys.at(key));
    }


    // 
    // RENDERABLE UTILS
    //

    bool keyExists(char const * key) {
        return (keys.find(key) != keys.end());
    }


    // 
    // GLTF
    //

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

    void setPoolNextFree(size_t startFrom) {
        // if (poolNextFree < startFrom) return;
        fprintf(stderr, "LOOKING FOR NEXT FREE....\n");
        for (size_t i = startFrom; i < poolSize; ++i) {
            fprintf(stderr, "item in pool at %zu is %s\n", i, at(i)->active?"ACTIVE":"NOT ACTIVE");
            if (!at(i)->active) {
                poolNextFree = i;
                return;
            }
        }
        // no free slots found
        poolNextFree = SIZE_MAX;
    }

    static constexpr size_t poolSize = 16;
    Renderable pool[poolSize];
    size_t poolNextFree = 0;
    std::vector<size_t> renderables;
    std::unordered_map<std::string, size_t> keys;
};
