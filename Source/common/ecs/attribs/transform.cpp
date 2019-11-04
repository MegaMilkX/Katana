#include "transform.hpp"

#include "../world.hpp"

ecsTransform::~ecsTransform() {
    std::set<ecsTransform*> children_copy = _children;
    for(auto c : children_copy) {
        c->setParent(0);
    }
}

void ecsTransform::dirty() {
    if(!_dirty) {
        _dirty = true;
        for(auto c : _children) {
            c->dirty();
        }
    }
}
bool ecsTransform::isDirty() {
    return _dirty;
}

void ecsTransform::lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up_vec, float f) {
    gfxm::mat3 mat_rotation (1.0f);
    gfxm::vec3 forward = gfxm::normalize(getWorldPosition() - tgt);
    gfxm::vec3 up = gfxm::normalize(up_vec);
    gfxm::vec3 right = gfxm::normalize(gfxm::cross(forward, up));

    mat_rotation[0] = right;
    mat_rotation[1] = up;
    mat_rotation[2] = -forward;

    gfxm::quat q = gfxm::to_quat(mat_rotation);

    setRotation(gfxm::slerp(getRotation(), q, f));
}

void ecsTransform::translate(float x, float y, float z) {
    translate(gfxm::vec3(x, y, z)); 
}
void ecsTransform::translate(const gfxm::vec3& vec) {
    _position = _position + vec;
    dirty();
}
void ecsTransform::rotate(float angle, float axisX, float axisY, float axisZ) {
    rotate(angle, gfxm::vec3(axisX, axisY, axisZ));
}
void ecsTransform::rotate(float angle, const gfxm::vec3& axis) {
    rotate(gfxm::angle_axis(angle, axis));
}
void ecsTransform::rotate(const gfxm::quat& q) {
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void ecsTransform::setPosition(float x, float y, float z) {
    setPosition(gfxm::vec3(x, y, z)); 
}
void ecsTransform::setPosition(const gfxm::vec3& position) {
    _position = position; 
    dirty(); 
}
void ecsTransform::setRotation(float x, float y, float z) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); 
    dirty(); 
}
void ecsTransform::setRotation(gfxm::vec3 euler) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); 
    dirty(); 
}
void ecsTransform::setRotation(const gfxm::quat& rotation) {
    _rotation = rotation; 
    dirty(); 
}
void ecsTransform::setRotation(float x, float y, float z, float w) {
    setRotation(gfxm::quat(x, y, z, w)); 
}
void ecsTransform::setScale(float s) {
    setScale(gfxm::vec3(s, s, s)); 
}
void ecsTransform::setScale(float x, float y, float z) {
    setScale(gfxm::vec3(x, y, z)); 
}
void ecsTransform::setScale(const gfxm::vec3& s) {
    _scale = s; dirty(); 
}
void ecsTransform::setTransform(gfxm::mat4 t) {
    _position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
    gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
    _rotation = gfxm::to_quat(rotMat);
    gfxm::vec3 right = t[0];
    gfxm::vec3 up = t[1];
    gfxm::vec3 back = t[2];
    _scale = gfxm::vec3(right.length(), up.length(), back.length());
    dirty();
}

const gfxm::vec3& ecsTransform::getPosition() const {
    return _position;
}
const gfxm::quat& ecsTransform::getRotation() const {
    return _rotation;
}
const gfxm::vec3& ecsTransform::getScale() const {
    return _scale;
}
gfxm::vec3        ecsTransform::getEulerAngles() {
    return gfxm::to_euler(_rotation);
}
gfxm::vec3        ecsTransform::getWorldPosition() {
    return getWorldTransform()[3];
}
gfxm::quat        ecsTransform::getWorldRotation() {
    gfxm::quat q;
    if(getParent())
        q = getParent()->getWorldRotation() * _rotation;
    else
        q = _rotation;
    return q;
}
gfxm::vec3        ecsTransform::getWorldScale() {
    gfxm::vec3 s;
    if(getParent()) {
        auto ps = getParent()->getWorldScale();
        s = gfxm::vec3(ps.x * _scale.x, ps.y * _scale.y, ps.z * _scale.z);
    }
    else
        s = _scale;
    return s;
}

gfxm::vec3 ecsTransform::right()
{ return getWorldTransform()[0]; }
gfxm::vec3 ecsTransform::up()
{ return getWorldTransform()[1]; }
gfxm::vec3 ecsTransform::back()
{ return getWorldTransform()[2]; }
gfxm::vec3 ecsTransform::left()
{ return -right(); }
gfxm::vec3 ecsTransform::down()
{ return -up(); }
gfxm::vec3 ecsTransform::forward()
{ return -back(); }

gfxm::mat4        ecsTransform::getLocalTransform() const {
    return 
        gfxm::translate(gfxm::mat4(1.0f), _position) * 
        gfxm::to_mat4(_rotation) * 
        gfxm::scale(gfxm::mat4(1.0f), _scale);
}
gfxm::mat4        ecsTransform::getParentTransform() {
    if(getParent())
        return getParent()->getWorldTransform();
    else
        return gfxm::mat4(1.0f);
}
const gfxm::mat4& ecsTransform::getWorldTransform() {
    if(isDirty()) {
        _dirty = false;
        gfxm::mat4 localTransform = getLocalTransform();           
        if(getParent())
            _world_transform = getParent()->getWorldTransform() * localTransform;
        else
            _world_transform = localTransform;
    }
    return _world_transform;
}

void ecsTransform::setParent(ecsTransform* p) {
    if(_parent) {
        _parent->_children.erase(this);
    }
    _parent = p;
    if(_parent) {
        _parent->_children.insert(this);
    }
}
ecsTransform* ecsTransform::getParent() const {
    return _parent;
}


void ecsTransform::onGui(ecsWorld* world, entity_id ent) {
    if(ImGui::DragFloat3("Translation", (float*)&_position, 0.001f)) {
        dirty();
        world->signalAttribUpdate<ecsTransform>(ent);
    }
    if(ImGui::DragFloat4("Quaternion", (float*)&_rotation, 0.001f)) {
        dirty();
        world->signalAttribUpdate<ecsTransform>(ent);
    }
    if(ImGui::DragFloat3("Scale", (float*)&_scale, 0.001f)) {
        dirty();
        world->signalAttribUpdate<ecsTransform>(ent);
    }
}