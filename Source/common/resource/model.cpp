#include "model.hpp"


void ModelNode::setName(const char* name) {
    _name = name;
}
const std::string& ModelNode::getName() const {
    return _name;
}

ModelNode* ModelNode::createChild() {
    ModelNode* node = new ModelNode();
    node->_parent = this;
    _children.insert(node);
    return node;
}
size_t             ModelNode::childCount() {
    return _children.size();
}
ModelNode* ModelNode::getChild(size_t id) {
    auto it = _children.begin();
    std::advance(it, id);
    return *it;
}

void ModelNode::dirty() {
    if(!_dirty) {
        _dirty = true;
        for(auto c : _children) {
            c->dirty();
        }
    }
}
bool ModelNode::isDirty() {
    return _dirty;
}

void ModelNode::lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up_vec, float f) {
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

void ModelNode::translate(float x, float y, float z) {
    translate(gfxm::vec3(x, y, z)); 
}
void ModelNode::translate(const gfxm::vec3& vec) {
    _position = _position + vec;
    dirty();
}
void ModelNode::rotate(float angle, float axisX, float axisY, float axisZ) {
    rotate(angle, gfxm::vec3(axisX, axisY, axisZ));
}
void ModelNode::rotate(float angle, const gfxm::vec3& axis) {
    rotate(gfxm::angle_axis(angle, axis));
}
void ModelNode::rotate(const gfxm::quat& q) {
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void ModelNode::setPosition(float x, float y, float z) {
    setPosition(gfxm::vec3(x, y, z)); 
}
void ModelNode::setPosition(const gfxm::vec3& position) {
    _position = position; 
    dirty(); 
}
void ModelNode::setRotation(float x, float y, float z) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); 
    dirty(); 
}
void ModelNode::setRotation(gfxm::vec3 euler) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); 
    dirty(); 
}
void ModelNode::setRotation(const gfxm::quat& rotation) {
    _rotation = rotation; 
    dirty(); 
}
void ModelNode::setRotation(float x, float y, float z, float w) {
    setRotation(gfxm::quat(x, y, z, w)); 
}
void ModelNode::setScale(float s) {
    setScale(gfxm::vec3(s, s, s)); 
}
void ModelNode::setScale(float x, float y, float z) {
    setScale(gfxm::vec3(x, y, z)); 
}
void ModelNode::setScale(const gfxm::vec3& s) {
    _scale = s; dirty(); 
}
void ModelNode::setTransform(gfxm::mat4 t) {
    _position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
    gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
    _rotation = gfxm::to_quat(rotMat);
    gfxm::vec3 right = t[0];
    gfxm::vec3 up = t[1];
    gfxm::vec3 back = t[2];
    _scale = gfxm::vec3(right.length(), up.length(), back.length());
    dirty();
}

const gfxm::vec3& ModelNode::getPosition() const {
    return _position;
}
const gfxm::quat& ModelNode::getRotation() const {
    return _rotation;
}
const gfxm::vec3& ModelNode::getScale() const {
    return _scale;
}
gfxm::vec3        ModelNode::getEulerAngles() {
    return gfxm::to_euler(_rotation);
}
gfxm::vec3        ModelNode::getWorldPosition() {
    return getWorldTransform()[3];
}
gfxm::quat        ModelNode::getWorldRotation() {
    gfxm::quat q;
    if(getParent())
        q = getParent()->getWorldRotation() * _rotation;
    else
        q = _rotation;
    return q;
}
gfxm::vec3        ModelNode::getWorldScale() {
    gfxm::vec3 s;
    if(getParent()) {
        auto ps = getParent()->getWorldScale();
        s = gfxm::vec3(ps.x * _scale.x, ps.y * _scale.y, ps.z * _scale.z);
    }
    else
        s = _scale;
    return s;
}

gfxm::vec3 ModelNode::right()
{ return getWorldTransform()[0]; }
gfxm::vec3 ModelNode::up()
{ return getWorldTransform()[1]; }
gfxm::vec3 ModelNode::back()
{ return getWorldTransform()[2]; }
gfxm::vec3 ModelNode::left()
{ return -right(); }
gfxm::vec3 ModelNode::down()
{ return -up(); }
gfxm::vec3 ModelNode::forward()
{ return -back(); }

gfxm::mat4        ModelNode::getLocalTransform() const {
    return 
        gfxm::translate(gfxm::mat4(1.0f), _position) * 
        gfxm::to_mat4(_rotation) * 
        gfxm::scale(gfxm::mat4(1.0f), _scale);
}
gfxm::mat4        ModelNode::getParentTransform() {
    if(getParent())
        return getParent()->getWorldTransform();
    else
        return gfxm::mat4(1.0f);
}
const gfxm::mat4& ModelNode::getWorldTransform() {
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

void ModelNode::setParent(ModelNode* p) {
    if(_parent) {
        _parent->_children.erase(this);
    }
    _parent = p;
    if(_parent) {
        _parent->_children.insert(this);
    }
}
ModelNode* ModelNode::getParent() const {
    return _parent;
}