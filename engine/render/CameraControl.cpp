#include "CameraControl.h"
#include "../MrManager.h"

void CameraControl::mousePosEvent(Event const & e) {
    if (mm.mouseIsDown) {
        if (mm.camera.projType == Camera::ProjType::Persp) {
            mm.camera.yaw -= (e.x - mm.mousePrevPos.x) * 0.01;
            mm.camera.pitch -= (e.y - mm.mousePrevPos.y) * 0.01;
            mm.camera.updatePosFromDistancePitchYaw();
        }
        else if (mm.camera.projType == Camera::ProjType::Ortho) {
            float winDistRatio = mm.camera.distance / (float)mm.windowSize.h;
            mm.camera.target.x -= (e.x - mm.mousePrevPos.x) * winDistRatio * orthoPanScale;
            mm.camera.target.y += (e.y - mm.mousePrevPos.y) * winDistRatio * orthoPanScale;
            mm.camera.updateProjection();
        }
    }
}

void CameraControl::scrollEvent(Event const & e) {
    if (mm.camera.projType == Camera::ProjType::Persp) {
        mm.camera.distance -= e.y;
        mm.camera.updatePosFromDistancePitchYaw();
    }
    else if (mm.camera.projType == Camera::ProjType::Ortho) {
        mm.camera.distance -= e.y * orthoScrollScale;
        if (mm.camera.distance < 0.1f) mm.camera.distance = 0.1f;
        mm.camera.updateProjection();
    }
}
