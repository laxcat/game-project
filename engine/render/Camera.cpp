#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "../memory/Gobj.h"

#if DEV_INTERFACE
char const * Camera::projTypeStr(int index) {
    if (index == -1) index = (int)projType;
    static char const * strs[] = {"Persepctive", "Orthographic"};
    return strs[index];
}
int Camera::projTypeCount() {
    return 2;
}
#endif // DEV_INTERFACE

void Camera::reset() {
    if (projType == ProjType::Persp) {
        if (perspResetFn) {
            perspResetFn(*this);
        }
        else {
            target      = {0.f, 0.f, 0.f};
            distance    = 5;
            pitch       = 0;
            yaw         = 0;
            fov         = (float)M_PI/4.f;
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

void Camera::init(size2 windowSize) {
    setRatio(windowSize);
    reset();

    uniform = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
}

void Camera::shutdown() {
    bgfx::destroy(uniform);
}

void Camera::updateProjection() {
    if (projType == ProjType::Persp) {
        projMat = glm::perspective(fov, windowRatio, 0.005f, 100000.f);
        fovx = 2.f * atan2f(tanf(fov/2.f), windowInvRatio);
    }
    else if (projType == ProjType::Ortho) {
        float halfW = distance/2.f * windowRatio;
        float halfH = distance/2.f;

        // This makes no sense. What i want is RH (y-up), which means +z should be "near".
        // But it only works if we use orthoRH_ZO, and setup the near far "backwards", with -z "near".
        // Other functions seem to clip all +z (or -z).
        // Only with this setup could I get +/-z to not clip AND +z as "near",
        //     but note near/far parameters are "backwards".
        // Could be something to do w this?: https://stackoverflow.com/a/52887566/560384
        projMat = glm::orthoRH_ZO(
            // left         // right
            target.x-halfW, target.x+halfW,
            // bottom       // top
            target.y-halfH, target.y+halfH,
            // "near" (?)   // "far" (?)
            -1000.f,        1000.f
        );

        viewMat = glm::mat4{1.f};
    }
}

void Camera::updatePosFromDistancePitchYaw() {
    float xy = cosf(-pitch) * distance;
    pos.x = target.x + sinf(yaw) * xy;
    pos.y = target.y + sinf(-pitch) * distance;
    pos.z = target.z + cosf(yaw) * xy;
    viewMat = glm::lookAt(pos, target, up);
}

void Camera::updateView() {
    viewMat = glm::lookAt(pos, target, up);
}

void Camera::setRatio(size2 windowSize) {
    windowRatio = float(windowSize.w) / float(windowSize.h);
    windowInvRatio = 1.f/windowRatio;
    updateProjection();
}

void Camera::preDraw() {
    bgfx::setUniform(uniform, (float *)&pos);
}

void Camera::zoomTo(Gobj * gobj) {
    target = gobj->bounds.center();
    float boundsSphereRadius = glm::length(target - gobj->bounds.min);
    distance = boundsSphereRadius / tanf(fov/2.f) * 1.2;
    updatePosFromDistancePitchYaw();
}
