#pragma once
#include "Renderable.h"


class Camera {
public:
    enum ControlType {
        Free,
        Orbit,
    };

    enum CursorMode {
        NormalCursor,
        RawMotion,
    };

    // const
    glm::vec3 const up = {0.f, 1.f, 0.f};

    // set by ctor
    float windowRatio;

    // updated by user
    ControlType controlType = Free;
    glm::vec3 target;
    float distance;
    float pitch;
    float yaw;
    float fov;

    // keyboard/joystick motion
    // glm::vec3 motionTarget;
    glm::vec3 motionActual;
    // float motionTargetFactor = 0.001f;
    bool invertFreeY = false;

    // updated by calculation (update functions)
    glm::vec3 pos;
    glm::mat4 projMat;
    glm::mat4 viewMat;

    Renderable * targetR = nullptr;

    bgfx::UniformHandle uniform;

    CursorMode cursorMode;

    void reset();
    void init(size2 windowSize);
    void updateProjection();
    void updatePosFromDistancePitchYaw();
    void updateTargetFromPitchYaw();
    void update();
    void preDraw();
    void setCursorMode(CursorMode mode);
};
