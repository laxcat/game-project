#include "InputSys.h"
#include "../MrManager.h"
#include "glfw.h"


void InputSys::mousePosEvent(double x, double y) {
    prevPos = pos;
    pos.x = x;
    pos.y = y;

    if (isDown) {
        if (mm.camera.projType == Camera::ProjType::Persp) {
            mm.camera.yaw -= (x - prevPos.x) * 0.01;
            mm.camera.pitch -= (y - prevPos.y) * 0.01;
            mm.camera.updatePosFromDistancePitchYaw();
        }
        else if (mm.camera.projType == Camera::ProjType::Ortho) {
            float winDistRatio = mm.camera.distance / (float)mm.windowSize.h;
            mm.camera.target.x -= (x - prevPos.x) * winDistRatio * orthoPanScale.x;
            mm.camera.target.y += (y - prevPos.y) * winDistRatio * orthoPanScale.y;
            mm.camera.updateProjection();
        }
    }
}

void InputSys::mouseButtonEvent(int button, int action, int mods) {
    isDown = action;
    if (isDown) {
        double tx, ty;
        glfwGetCursorPos(mm.window, &tx, &ty);
        pos.x = tx;
        pos.y = ty;
        downPos = pos;
    }
    else {
        upPos = pos;
    }
}

void InputSys::scrollEvent(double x, double y) {
    if (mm.camera.projType == Camera::ProjType::Persp) {
        mm.camera.distance -= y;
        mm.camera.updatePosFromDistancePitchYaw();
    }
    else if (mm.camera.projType == Camera::ProjType::Ortho) {
        mm.camera.distance -= y * orthoScrollScale;
        if (mm.camera.distance < 0.1f) mm.camera.distance = 0.1f;
        mm.camera.updateProjection();
    }
}
