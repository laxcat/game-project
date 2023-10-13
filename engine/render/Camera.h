#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <bgfx/bgfx.h>
#include "../common/types.h"

class Gobj;

class Camera {
public:
    // const
    glm::vec3 const up = {0.f, 1.f, 0.f};

    // set by init
    float windowRatio;
    float windowInvRatio;

    // updated by user
    glm::vec3 target;
    float distance;
    float pitch;
    float yaw;
    float fov; // y

    // updated by calculation (update functions)
    glm::vec3 pos;
    glm::mat4 projMat;
    glm::mat4 viewMat;
    float fovx;

    enum class ProjType {
        Persp,
        Ortho,
    };
    ProjType projType = ProjType::Persp;

    bgfx::UniformHandle uniform;

    #if DEV_INTERFACE
    char const * projTypeStr(int index = -1);
    int projTypeCount();
    #endif // DEV_INTERFACE

    using reset_fn_t = void (*)(Camera &);
    reset_fn_t perspResetFn = nullptr;
    reset_fn_t orthoResetFn = nullptr;

    void reset();
    void init(size2 windowSize);
    void shutdown();
    void updateProjection();
    void updatePosFromDistancePitchYaw();
    void updateView();
    void setRatio(size2 windowSize);
    void preDraw();
    void zoomTo(Gobj * gobj);
};
