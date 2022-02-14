#include "InputSys.h"
#include "../MrManager.h"
#include <glm/gtx/string_cast.hpp>


void InputSys::mousePosEvent(double x, double y) {
    prevPos = pos;
    pos.x = x;
    pos.y = y;
    auto & c = mm.camera;

    if (isDown && c.controlType == Camera::Orbit) {
        c.yaw -= (x - prevPos.x) * 0.01;
        c.pitch -= (y - prevPos.y) * 0.01;
        c.updatePosFromDistancePitchYaw();
    }

    else if (c.controlType == Camera::Free && c.cursorMode == Camera::RawMotion) {
        c.yaw   -= (x - prevPos.x) * 0.005;
        c.pitch -= (y - prevPos.y) * 0.005 * ((c.invertFreeY)?-1:1);
        c.updateTargetFromPitchYaw();
    }
}

void InputSys::mouseButtonEvent(int button, int action, int mods) {
    auto & c = mm.camera;

    isDown = action;
    if (isDown) {
        downPos = pos;

        if (c.controlType == Camera::Free) {
            c.setCursorMode(Camera::RawMotion);
        }
    }
    else {
        upPos = pos;
    }
}

void InputSys::scrollEvent(double x, double y) {
    auto & c = mm.camera;
    if (c.controlType == Camera::Orbit) {
        c.distance -= y;
        c.updatePosFromDistancePitchYaw();
    }
}

void InputSys::keyEvent(int key, int scancode, int action, int mods) {
    auto & c = mm.camera;
    auto & k = mm.keys;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        c.setCursorMode(Camera::NormalCursor);
    }

    // print("KEY PRESS %d (%s)\n", key, (action == GLFW_PRESS)?"PRESS":(action==GLFW_RELEASE))

    if (c.controlType == Camera::Free && 
        c.cursorMode == Camera::RawMotion &&
        (action == GLFW_PRESS || action == GLFW_RELEASE)) {
        if      (key == k.moveForward)  c.motionActual.z = (action == GLFW_PRESS)? -1 : 0;
        else if (key == k.moveLeft)     c.motionActual.x = (action == GLFW_PRESS)? -1 : 0;
        else if (key == k.moveBackward) c.motionActual.z = (action == GLFW_PRESS)? +1 : 0;
        else if (key == k.moveRight)    c.motionActual.x = (action == GLFW_PRESS)? +1 : 0;
        else if (key == k.moveUp)       c.motionActual.y = (action == GLFW_PRESS)? +1 : 0;
        else if (key == k.moveDown)     c.motionActual.y = (action == GLFW_PRESS)? -1 : 0;
    }
}

void InputSys::tick() {
    auto & c = mm.camera;

    if (c.controlType == Camera::Free && c.cursorMode == Camera::RawMotion) {
        // c.pos += c.motionActual * 0.001f;
        // c.updateTargetFromPitchYaw();
        // print("motionActual %s, cam pos %s\n", 
        //     // glm::to_string(c.motionTarget).c_str(),
        //     glm::to_string(c.motionActual).c_str(),
        //     glm::to_string(c.pos).c_str()
        // );
    }

}