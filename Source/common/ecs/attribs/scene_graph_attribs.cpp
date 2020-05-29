#include "scene_graph_attribs.hpp"

#include "../systems/scene_graph.hpp"


void ecsTranslation::dirty() {
    if(system) system->setDirtyIndex(dirty_index);
}

void ecsTranslation::translate(float x, float y, float z) {
    translate(gfxm::vec3(x, y, z)); 
}
void ecsTranslation::translate(const gfxm::vec3& vec) {
    position = position + vec;
    dirty();
}

void ecsTranslation::setPosition(float x, float y, float z) {
    setPosition(gfxm::vec3(x, y, z));
}
void ecsTranslation::setPosition(const gfxm::vec3& position) {
    this->position = position;
    dirty();
}

const gfxm::vec3& ecsTranslation::getPosition() const {
    return position;
}


void ecsRotation::dirty() {
    if(system) system->setDirtyIndex(dirty_index);
}

void ecsRotation::rotate(float angle, float axisX, float axisY, float axisZ) {
    rotate(angle, gfxm::vec3(axisX, axisY, axisZ));
}
void ecsRotation::rotate(float angle, const gfxm::vec3& axis) {
    rotate(gfxm::angle_axis(angle, axis));
}
void ecsRotation::rotate(const gfxm::quat& q) {
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void ecsRotation::setRotation(float x, float y, float z) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); 
    dirty(); 
}
void ecsRotation::setRotation(gfxm::vec3 euler) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); 
    dirty(); 
}
void ecsRotation::setRotation(const gfxm::quat& rotation) {
    _rotation = rotation; 
    dirty(); 
}
void ecsRotation::setRotation(float x, float y, float z, float w) {
    setRotation(gfxm::quat(x, y, z, w)); 
}

const gfxm::quat& ecsRotation::getRotation() const {
    return _rotation;
}


void ecsScale::dirty() {
    if(system) system->setDirtyIndex(dirty_index);
}

void ecsScale::setScale(float s) {
    setScale(gfxm::vec3(s, s, s)); 
}
void ecsScale::setScale(float x, float y, float z) {
    setScale(gfxm::vec3(x, y, z)); 
}
void ecsScale::setScale(const gfxm::vec3& s) {
    _scale = s; 
    dirty(); 
}

const gfxm::vec3& ecsScale::getScale() const {
    return _scale;
}

