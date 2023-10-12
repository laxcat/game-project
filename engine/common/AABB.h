#include <glm/vec3.hpp>
#include <float.h>
#include <array>

struct AABB {
    glm::vec3 min{+FLT_MAX, +FLT_MAX, +FLT_MAX};
    glm::vec3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    // given y-up, 0=min, 6=max
    //    7------6
    //   /|     /|
    //  4------5 |
    //  | |    | |
    //  | 3----|-2 <--far
    //  |/     |/
    //  0------1   <--near
    void fillCubePoints(glm::vec3 * points) const {
        // bottom (min y)
        points[0] = min;
        points[1] = glm::vec3{max.x, min.y, min.z};
        points[2] = glm::vec3{max.x, min.y, max.z};
        points[3] = glm::vec3{min.x, min.y, max.z};
        // top (max y)
        points[4] = glm::vec3{min.x, max.y, min.z};
        points[5] = glm::vec3{max.x, max.y, min.z};
        points[6] = max;
        points[7] = glm::vec3{min.x, max.y, max.z};
    }

    glm::vec3 center() const {
        return min + (max - min) / 2.f;
    }

    uint16_t renderHandleIndex = UINT16_MAX;
    uint16_t renderHandleVertex = UINT16_MAX;
};
