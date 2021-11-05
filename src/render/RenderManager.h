#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Renderable.h"
#include "GLTFLoader.h"


class RenderManager {
public:
    // 
    // MANAGER LIFECYCLE
    //

    void init();

    void draw() {
        // fprintf(stderr, "renderable count: %zu\n", renderables.size());
        for (auto i : renderables) {
            at(i)->draw();
        }
    }

    void shutdown() {
        Renderable * r;
        for (size_t i = 0; i < poolSize; ++i) {
            r = at(i);
            r->active = false;
        }
    }


    // 
    // RENDERABLE LIFECYCLE
    //

    size_t create(bgfx::ProgramHandle program, char const * key = "") {
        if (poolNextFree >= poolSize) {
            fprintf(stderr, "WARNING: did not create renderable. Pool full.");
            return SIZE_MAX;
        }
        if (strcmp(key, "") != 0 && keyExists(key)) {
            fprintf(stderr, "WARNING: did not create renderable. Key already in use.");
            return SIZE_MAX;
        }

        size_t index = poolNextFree;
        fprintf(stderr, "CREATING AT %zu (%p)\n", index, &pool[index]);
        new (&pool[index]) Renderable();
        Renderable * renderable = at(index);
        renderable->program = program;
        renderable->active = true;
        renderables.push_back(index);
        if (strcmp(key, "") != 0) {
            renderable->key = key;
            keys.emplace(key, index);
        }

        setPoolNextFree(index+1);
        return index;
    }

    size_t createFromGLTF(char const * filename = "") { 
        auto renderable = create(gltfProgram, filename);
        gltfLoader.load(filename, renderable);
        return renderable;
    }

    Renderable * at(int index) {
        assert(index < poolSize && "Out of bounds.");
        return &pool[index];
    }

    Renderable * at(char const * key) {
        return at(keys.at(key));
    }

    void destroy(size_t index) {
        auto * r = at(index);
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


private:

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
    GLTFLoader gltfLoader;
    bgfx::ProgramHandle gltfProgram;
};
