#include "OriginWidget.h"
#include "../MrManager.h"
#include "../primitives/Cube.h"


float OriginWidget::scale;

Renderable * OriginWidget::create() {
    auto ret = mm.rendSys.create(mm.rendSys.unlitProgram, Renderable::OriginWidgetKey);
    Cube::allocateBufferWithCount(ret, 3);

    ret->materials.push_back({.baseColor = {1.f, 1.f, 1.f, 1.f}});
    
    float t = 0.05f;
    Cube::create(ret, 0, glm::vec3{1.0f, t, t}, glm::vec3{0.5f, 0.0f, 0.0f}, {0xff0000ff}, 0);
    Cube::create(ret, 1, glm::vec3{t, 1.0f, t}, glm::vec3{0.0f, 0.5f, 0.0f}, {0xff00ff00}, 0);
    Cube::create(ret, 2, glm::vec3{t, t, 1.0f}, glm::vec3{0.0f, 0.0f, 0.5f}, {0xffff0000}, 0);

    scale = 1.f;

    return ret;
}

void OriginWidget::setScale(float s){
    if (!mm.rendSys.keyExists(Renderable::OriginWidgetKey)) return;
    scale = s;
    auto r = mm.rendSys.at(Renderable::OriginWidgetKey);
    r->instances[0].model = glm::scale(glm::mat4{1.f}, glm::vec3{s});
}

void OriginWidget::updateModelFromScale(){
    setScale(scale);
}
