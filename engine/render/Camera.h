#pragma once
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>


class Camera {
public:
    // const
    glm::vec3 const up = {0.f, 1.f, 0.f};

    // set by ctor
    float windowRatio;

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

    // Renderable * targetRenderable = nullptr;
    // size_t targetRenderableIndex = 0;

    bgfx::UniformHandle uniform;

    void reset() {
        target      = {0.f, 0.f, 0.f};
        distance    = 27;
        pitch       = 0;
        yaw         = 0;
        fov         = (float)M_PI/6.f;

        updateProjection();
        updatePosFromDistancePitchYaw();
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
        projMat = glm::perspective(fov, windowRatio, 0.05f, 500.f);
    }

    void updatePosFromDistancePitchYaw() {
        // renderable target if set
        glm::vec3 t = target;
        // if (targetRenderable) t += glm::vec3(targetRenderable->model[targetRenderableIndex][3]);

        float xy = cosf(-pitch) * distance;
        pos.x = t.x + sinf(yaw) * xy;
        pos.y = t.y + sinf(-pitch) * distance;
        pos.z = t.z + cosf(yaw) * xy;
        // printf("POS %f %f %f\n", pos.x, pos.y, pos.z);
        viewMat = glm::lookAt(pos, t, up);
    }

    void updateView() {
        // printf("POS %f %f %f\n", pos.x, pos.y, pos.z);
        viewMat = glm::lookAt(pos, target, up);
    }

    void setRatio(size2 windowSize) {
        windowRatio = float(windowSize.w) / float(windowSize.h);
        updateProjection();
    }

    void preDraw() {
        bgfx::setUniform(uniform, (float *)&pos);
    }
};
