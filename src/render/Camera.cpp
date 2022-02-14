#include "Camera.h"
#include <string_view>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "../develop/print.h"
#include "../MrManager.h"


void Camera::reset() {
    pos         = {0.0f, 0.0f, 3.0f};
    target      = {0.0f, 0.0f, 0.0f};
    distance    = 3;
    pitch       = 0;
    yaw         = 0;
    fov         = (float)M_PI/3.f;
    cursorMode  = NormalCursor;

    updateProjection();
    if (controlType == Orbit) {
        updatePosFromDistancePitchYaw();
        print("POS %s\n", glm::to_string(pos).c_str());
    }
    else if (controlType == Free && cursorMode == RawMotion) {
        updateTargetFromPitchYaw();
        print("POS %s\n", glm::to_string(pos).c_str());
    }
    update();

    // print("ControlTypes ==== %s %s\n", ControlTypeItemStrings[0], ControlTypeItemStrings[1]);
}

void Camera::init(size2 windowSize) {
    windowRatio = float(windowSize.w) / float(windowSize.h);
    reset();

    uniform = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
}

void Camera::updateProjection() {
    projMat = glm::perspective(fov, windowRatio, 0.05f, 500.f);
}

void Camera::updatePosFromDistancePitchYaw() {
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

void Camera::updateTargetFromPitchYaw() {
    distance = 1;

    target.y = pos.y - sinf(-pitch);
    float xzHyp = cosf(-pitch);
    target.x = pos.x + sinf(-yaw);
    target.z = pos.z - cosf(-yaw);
    viewMat = glm::lookAt(pos, target, up);
}

void Camera::update() {
    viewMat = glm::lookAt(pos, target, up);
}

void Camera::preDraw() {
    bgfx::setUniform(uniform, glm::value_ptr(pos));
}

void Camera::setCursorMode(CursorMode mode) {
    if (mode == cursorMode) return;
    cursorMode = mode;
    if (mode == NormalCursor) {
        print("CURSOR NORMAL MODE\n");
        glfwSetInputMode(mm.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(mm.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
    else if (mode == RawMotion) {
        print("CURSOR RAW MOTION\n");
        glfwSetInputMode(mm.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(mm.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}
