#pragma once
#include <string_view>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Renderable.h"


class Camera {
public:
    enum ControlType {
        Free,
        Orbit,
    };

    // const
    glm::vec3 const up = {0.f, 1.f, 0.f};

    // set by ctor
    float windowRatio;

    // updated by user
    ControlType controlType;
    glm::vec3 target;
    float distance;
    float pitch;
    float yaw;
    float fov;

    // updated by calculation (update functions)
    glm::vec3 pos;
    glm::mat4 projMat;
    glm::mat4 viewMat;

    Renderable * targetR = nullptr;

    bgfx::UniformHandle uniform;

    void reset() {
        controlType = Orbit;
        pos         = {0.0f, 0.0f, -10.0f};
        target      = {0.0f, 0.0f, 0.0f};
        distance    = 3;
        pitch       = 0;
        yaw         = 0;
        fov         = (float)M_PI/3.f;

        updateProjection();
        if (controlType == Orbit) {
            updatePosFromDistancePitchYaw();
        }
        else {
            update();
        }

        // print("ControlTypes ==== %s %s\n", ControlTypeItemStrings[0], ControlTypeItemStrings[1]);
    }

    void init(size2 windowSize) {
        windowRatio = float(windowSize.w) / float(windowSize.h);
        reset();

        uniform = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
    }

    void updateProjection() {
        projMat = glm::perspective(fov, windowRatio, 0.05f, 500.f);
    }

    void updatePosFromDistancePitchYaw() {
        // renderable target if set
        glm::vec3 t = target;
        if (targetR) t += glm::vec3(targetR->model[3]);

        float xy = cosf(-pitch) * distance;
        pos.x = t.x + sinf(yaw) * xy;
        pos.y = t.y + sinf(-pitch) * distance;
        pos.z = t.z + cosf(yaw) * xy;
        // print("POS %f %f %f\n", pos.x, pos.y, pos.z);
        viewMat = glm::lookAt(pos, t, up);
    }

    void update() {
        viewMat = glm::lookAt(pos, target, up);
    }

    void preDraw() {
        bgfx::setUniform(uniform, glm::value_ptr(pos));
    }
};
