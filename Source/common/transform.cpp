#include "transform.hpp"

#include "scene/node.hpp"

TransformNode::~TransformNode() {
    setParent(0);
}

void TransformNode::dirty() {
    if(!_dirty) {
        _sync_id++;
        if (owner) {
            owner->invokeTransformCallback();
        }
    }
    _dirty = true;
    _frame_dirty = true;
    for(auto& c : _children) {
        c->dirty();
    }
}
bool TransformNode::isDirty() {
    return _dirty;
}

void TransformNode::lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up_vec, float f) {
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

void TransformNode::translate(float x, float y, float z) { 
    translate(gfxm::vec3(x, y, z)); 
}
void TransformNode::translate(const gfxm::vec3& vec) {
    _position = _position + vec;
    dirty();
}
void TransformNode::rotate(float angle, float axisX, float axisY, float axisZ) { 
    rotate(angle, gfxm::vec3(axisX, axisY, axisZ));
}
void TransformNode::rotate(float angle, const gfxm::vec3& axis) {
    rotate(gfxm::angle_axis(angle, axis));
}
void TransformNode::rotate(const gfxm::quat& q) {
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void TransformNode::setPosition(float x, float y, float z) { 
    setPosition(gfxm::vec3(x, y, z)); 
}
void TransformNode::setPosition(const gfxm::vec3& position) { 
    _position = position; 
    dirty(); 
}
void TransformNode::setRotation(float x, float y, float z) { 
    _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); 
    dirty(); 
}
void TransformNode::setRotation(gfxm::vec3 euler) { 
    _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); 
    dirty(); 
}
void TransformNode::setRotation(float x, float y, float z, float w) { 
    setRotation(gfxm::quat(x, y, z, w)); 
}
void TransformNode::setRotation(const gfxm::quat& rotation) { 
    _rotation = rotation; 
    dirty(); 
}
void TransformNode::setScale(float s) { 
    setScale(gfxm::vec3(s, s, s)); 
}
void TransformNode::setScale(float x, float y, float z) { 
    setScale(gfxm::vec3(x, y, z)); 
}
void TransformNode::setScale(const gfxm::vec3& s) { 
    _scale = s; dirty(); 
}
void TransformNode::setTransform(gfxm::mat4 t) {
    _position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
    gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
    _rotation = gfxm::to_quat(rotMat);
    gfxm::vec3 right = t[0];
    gfxm::vec3 up = t[1];
    gfxm::vec3 back = t[2];
    _scale = gfxm::vec3(right.length(), up.length(), back.length());
    dirty();
}

void TransformNode::setParent(TransformNode* p) {
    if(_parent) {
        _parent->_children.erase(this);
    }
    _parent = p;
    if(_parent) {
        _parent->_children.insert(this);
    }
}

const gfxm::vec3& TransformNode::getPosition() const {
    return _position;
}
const gfxm::quat& TransformNode::getRotation() const {
    return _rotation;
}
const gfxm::vec3& TransformNode::getScale() const {
    return _scale;
}
gfxm::vec3 TransformNode::getEulerAngles() {
    return gfxm::to_euler(_rotation);
}
gfxm::vec3 TransformNode::getWorldPosition() {
    return getWorldTransform()[3];
}
gfxm::quat TransformNode::getWorldRotation() {
    gfxm::quat q;
    if(getParent())
        q = getParent()->getWorldRotation() * _rotation;
    else
        q = _rotation;
    return q;
}
gfxm::vec3 TransformNode::getWorldScale() {
    gfxm::vec3 s;
    if(getParent()) {
        auto ps = getParent()->getWorldScale();
        s = gfxm::vec3(ps.x * _scale.x, ps.y * _scale.y, ps.z * _scale.z);
    }
    else
        s = _scale;
    return s;
}

gfxm::vec3 TransformNode::right()
{ return getWorldTransform()[0]; }
gfxm::vec3 TransformNode::up()
{ return getWorldTransform()[1]; }
gfxm::vec3 TransformNode::back()
{ return getWorldTransform()[2]; }
gfxm::vec3 TransformNode::left()
{ return -right(); }
gfxm::vec3 TransformNode::down()
{ return -up(); }
gfxm::vec3 TransformNode::forward()
{ return -back(); }

gfxm::mat4 TransformNode::getLocalTransform() const {
    return 
        gfxm::translate(gfxm::mat4(1.0f), _position) * 
        gfxm::to_mat4(_rotation) * 
        gfxm::scale(gfxm::mat4(1.0f), _scale);
}
gfxm::mat4 TransformNode::getParentTransform() {
    if(getParent())
        return getParent()->getWorldTransform();
    else
        return gfxm::mat4(1.0f);
}
const gfxm::mat4& TransformNode::getWorldTransform() {
    if(isDirty())
    {
        _dirty = false;
        gfxm::mat4 localTransform = getLocalTransform();           
        if(getParent())
            _transform = getParent()->getWorldTransform() * localTransform;
        else
            _transform = localTransform;
    }
    return _transform;
}

TransformNode* TransformNode::getParent() const {
    return _parent;
}

void TransformNode::setOwner(ktNode* n) {
    owner = n;
}

void TransformNode::_frameClean() {
    _frame_dirty = false;
    for(auto& c : _children) {
        c->_frameClean();
    }
}
bool TransformNode::_isFrameDirty() const {
    return _frame_dirty;
}