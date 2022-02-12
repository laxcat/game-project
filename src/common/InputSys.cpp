#include "InputSys.h"
#include "../MrManager.h"


void InputSys::mousePosEvent(double x, double y) {
    prevPos = pos;
    pos.x = x;
    pos.y = y;

    if (isDown) {
        mm.camera.yaw -= (x - prevPos.x) * 0.01;
        mm.camera.pitch -= (y - prevPos.y) * 0.01;
        mm.camera.updatePosFromDistancePitchYaw();
    }
}

void InputSys::mouseButtonEvent(int button, int action, int mods) {
    isDown = action;
    if (isDown) {
        downPos = pos;
    }
    else {
        upPos = pos;
    }
}

void InputSys::scrollEvent(double x, double y) {
    mm.camera.distance -= y;
    mm.camera.updatePosFromDistancePitchYaw();
}
