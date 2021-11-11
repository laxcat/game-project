#pragma once
#include <entt/entity/registry.hpp>
#include "Renderable.h"


class TestQuadSystem {
public:
    static entt::entity e;

    static size_t renderable;
    
    static void init();
    static void shutdown();

    static float * getColor3(int index, float * color);
    static void setColor3(int index, float * color);
};
