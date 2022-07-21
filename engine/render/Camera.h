#pragma once
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>

class Camera {
public:
    // const
    glm::vec3 const up = {0.f, 1.f, 0.f};

    // set by ctor
    float windowRatio;
    float windowInvRatio;

    // updated by user
    glm::vec3 target;
    float distance;
    float pitch;
    float yaw;
    float fov;

    // updated by calculation (update functions)
    glm::vec3 pos;
    glm::mat4 projMat;
    glm::mat4 viewMat;

    enum class ProjType {
        Persp,
        Ortho,
    };
    ProjType projType = ProjType::Persp;
    #if DEV_INTERFACE
    char const * projTypeStr(int index = -1) {
        if (index == -1) index = (int)projType;
        static char const * strs[] = {"Persepctive", "Orthographic"};
        return strs[index];
    }
    int projTypeCount() {
        return 2;
    }
    #endif // DEV_INTERFACE

    using reset_fn_t = void (*)(Camera &);
    reset_fn_t perspResetFn = nullptr;
    reset_fn_t orthoResetFn = nullptr;

    bgfx::UniformHandle uniform;

    void reset() {
        if (projType == ProjType::Persp) {
            if (perspResetFn) {
                perspResetFn(*this);
            }
            else {
                target      = {0.f, 0.f, 0.f};
                distance    = 27;
                pitch       = 0;
                yaw         = 0;
                fov         = (float)M_PI/6.f;
            }
            updateProjection();
            updatePosFromDistancePitchYaw();
        }
        else if (projType == ProjType::Ortho) {
            if (orthoResetFn) {
                orthoResetFn(*this);
            }
            else {
                target      = {0.f, 0.f, 0.f};
                distance    = 34;
                pitch       = 0;
                yaw         = 0;
                fov         = 0;
            }
            updateProjection();
        }
    }

    void init(size2 windowSize) {
        setRatio(windowSize);
        reset();

        uniform = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
    }

    void shutdown() {
        bgfx::destroy(uniform);
    }

    void updateProjection() {
        if (projType == ProjType::Persp) {
            projMat = glm::perspective(fov, windowRatio, 0.05f, 1000.f);
        }
        else if (projType == ProjType::Ortho) {
            float halfW = distance/2.f * windowRatio;
            float halfH = distance/2.f;
            projMat = glm::ortho(target.x-halfW, target.x+halfW, target.y-halfH, target.y+halfH, -1000.f, 1000.f);
            viewMat = glm::mat4{1.f};
        }
    }

    void updatePosFromDistancePitchYaw() {
        float xy = cosf(-pitch) * distance;
        pos.x = target.x + sinf(yaw) * xy;
        pos.y = target.y + sinf(-pitch) * distance;
        pos.z = target.z + cosf(yaw) * xy;
        viewMat = glm::lookAt(pos, target, up);
    }

    void updateView() {
        viewMat = glm::lookAt(pos, target, up);
    }

    void setRatio(size2 windowSize) {
        windowRatio = float(windowSize.w) / float(windowSize.h);
        windowInvRatio = 1.f/windowRatio;
        updateProjection();
    }

    void preDraw() {
        bgfx::setUniform(uniform, (float *)&pos);
    }
};
