#include "CameraControl.h"
#include "../MrManager.h"

void CameraControl::mousePosInput(Input const & i) {
    if (mm.mouseIsDown) {
        if (mm.camera->projType == Camera::ProjType::Persp) {
            mm.camera->yaw -= (i.x - mm.mousePrevPos.x) * 0.01;
            mm.camera->pitch -= (i.y - mm.mousePrevPos.y) * 0.01;

            if (mm.camera->pitch < pitchMin) {
                mm.camera->pitch = pitchMin;
            }
            else if (mm.camera->pitch > pitchMax) {
                mm.camera->pitch = pitchMax;
            }

            mm.camera->updatePosFromDistancePitchYaw();
        }
        else if (mm.camera->projType == Camera::ProjType::Ortho) {
            float winDistRatio = mm.camera->distance / (float)mm.windowSize.h;
            mm.camera->target.x -= (i.x - mm.mousePrevPos.x) * winDistRatio * orthoPanScale;
            mm.camera->target.y += (i.y - mm.mousePrevPos.y) * winDistRatio * orthoPanScale;
            mm.camera->updateProjection();
        }
    }
}

void CameraControl::scrollInput(Input const & i) {
    if (mm.camera->projType == Camera::ProjType::Persp) {
        mm.camera->distance -= i.scrolly;
        if (mm.camera->distance < distMin) {
            mm.camera->distance = distMin;
        }
        mm.camera->updatePosFromDistancePitchYaw();
    }
    else if (mm.camera->projType == Camera::ProjType::Ortho) {
        mm.camera->distance -= i.scrolly * orthoScrollScale;
        if (mm.camera->distance < distMin) {
            mm.camera->distance = distMin;
        }
        mm.camera->updateProjection();
    }
}
