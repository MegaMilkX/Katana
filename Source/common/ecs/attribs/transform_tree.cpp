#include "transform_tree.hpp"


void TransformTreeNode::setName(const char* name) {
    _name = name;
}
const std::string& TransformTreeNode::getName() const {
    return _name;
}

TransformTreeNode* TransformTreeNode::createChild() {
    TransformTreeNode* node = new TransformTreeNode();
    node->_parent = this;
    _children.insert(node);
    return node;
}
size_t             TransformTreeNode::childCount() {
    return _children.size();
}
TransformTreeNode* TransformTreeNode::getChild(size_t id) {
    auto it = _children.begin();
    std::advance(it, id);
    return *it;
}

void TransformTreeNode::dirty() {
    if(!_dirty) {
        _dirty = true;
        for(auto c : _children) {
            c->dirty();
        }
    }
}
bool TransformTreeNode::isDirty() {
    return _dirty;
}

void TransformTreeNode::lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up_vec, float f) {
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

void TransformTreeNode::translate(float x, float y, float z) {
    translate(gfxm::vec3(x, y, z)); 
}
void TransformTreeNode::translate(const gfxm::vec3& vec) {
    _position = _position + vec;
    dirty();
}
void TransformTreeNode::rotate(float angle, float axisX, float axisY, float axisZ) {
    rotate(angle, gfxm::vec3(axisX, axisY, axisZ));
}
void TransformTreeNode::rotate(float angle, const gfxm::vec3& axis) {
    rotate(gfxm::angle_axis(angle, axis));
}
void TransformTreeNode::rotate(const gfxm::quat& q) {
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void TransformTreeNode::setPosition(float x, float y, float z) {
    setPosition(gfxm::vec3(x, y, z)); 
}
void TransformTreeNode::setPosition(const gfxm::vec3& position) {
    _position = position; 
    dirty(); 
}
void TransformTreeNode::setRotation(float x, float y, float z) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); 
    dirty(); 
}
void TransformTreeNode::setRotation(gfxm::vec3 euler) {
    _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); 
    dirty(); 
}
void TransformTreeNode::setRotation(const gfxm::quat& rotation) {
    _rotation = rotation; 
    dirty(); 
}
void TransformTreeNode::setRotation(float x, float y, float z, float w) {
    setRotation(gfxm::quat(x, y, z, w)); 
}
void TransformTreeNode::setScale(float s) {
    setScale(gfxm::vec3(s, s, s)); 
}
void TransformTreeNode::setScale(float x, float y, float z) {
    setScale(gfxm::vec3(x, y, z)); 
}
void TransformTreeNode::setScale(const gfxm::vec3& s) {
    _scale = s; dirty(); 
}
void TransformTreeNode::setTransform(gfxm::mat4 t) {
    _position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
    gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
    _rotation = gfxm::to_quat(rotMat);
    gfxm::vec3 right = t[0];
    gfxm::vec3 up = t[1];
    gfxm::vec3 back = t[2];
    _scale = gfxm::vec3(right.length(), up.length(), back.length());
    dirty();
}

const gfxm::vec3& TransformTreeNode::getPosition() const {
    return _position;
}
const gfxm::quat& TransformTreeNode::getRotation() const {
    return _rotation;
}
const gfxm::vec3& TransformTreeNode::getScale() const {
    return _scale;
}
gfxm::vec3        TransformTreeNode::getEulerAngles() {
    return gfxm::to_euler(_rotation);
}
gfxm::vec3        TransformTreeNode::getWorldPosition() {
    return getWorldTransform()[3];
}
gfxm::quat        TransformTreeNode::getWorldRotation() {
    gfxm::quat q;
    if(getParent())
        q = getParent()->getWorldRotation() * _rotation;
    else
        q = _rotation;
    return q;
}
gfxm::vec3        TransformTreeNode::getWorldScale() {
    gfxm::vec3 s;
    if(getParent()) {
        auto ps = getParent()->getWorldScale();
        s = gfxm::vec3(ps.x * _scale.x, ps.y * _scale.y, ps.z * _scale.z);
    }
    else
        s = _scale;
    return s;
}

gfxm::vec3 TransformTreeNode::right()
{ return getWorldTransform()[0]; }
gfxm::vec3 TransformTreeNode::up()
{ return getWorldTransform()[1]; }
gfxm::vec3 TransformTreeNode::back()
{ return getWorldTransform()[2]; }
gfxm::vec3 TransformTreeNode::left()
{ return -right(); }
gfxm::vec3 TransformTreeNode::down()
{ return -up(); }
gfxm::vec3 TransformTreeNode::forward()
{ return -back(); }

gfxm::mat4        TransformTreeNode::getLocalTransform() const {
    return 
        gfxm::translate(gfxm::mat4(1.0f), _position) * 
        gfxm::to_mat4(_rotation) * 
        gfxm::scale(gfxm::mat4(1.0f), _scale);
}
gfxm::mat4        TransformTreeNode::getParentTransform() {
    if(getParent())
        return getParent()->getWorldTransform();
    else
        return gfxm::mat4(1.0f);
}
const gfxm::mat4& TransformTreeNode::getWorldTransform() {
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

void TransformTreeNode::setParent(TransformTreeNode* p) {
    if(_parent) {
        _parent->_children.erase(this);
    }
    _parent = p;
    if(_parent) {
        _parent->_children.insert(this);
    }
}
TransformTreeNode* TransformTreeNode::getParent() const {
    return _parent;
}