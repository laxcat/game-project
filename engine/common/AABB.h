#include <glm/vec3.hpp>
#include <float.h>

struct AABB {
    glm::vec3 min{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    glm::vec3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};
};
