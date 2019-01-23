#include "transform.hpp"
#include "scene.hpp"

#include <algorithm>

typedef gfxm::vec4 vec4;
typedef gfxm::vec3 vec3;
typedef gfxm::vec2 vec2;
typedef gfxm::mat4 mat4;
typedef gfxm::mat3 mat3;
typedef gfxm::quat quat;

void Transform::dirty()
{
    _dirty = true;
    for(size_t i = 0; i < getObject()->childCount(); ++i) {
        getObject()->getChild(i)->get<Transform>()->dirty();
    }
}

void Transform::translate(float x, float y, float z)
{ translate(gfxm::vec3(x, y, z)); }
void Transform::translate(const gfxm::vec3& vec)
{
    _position = _position + vec;
    dirty();
}
void Transform::rotate(float angle, float axisX, float axisY, float axisZ)
{ rotate(angle, gfxm::vec3(axisX, axisY, axisZ)); }
void Transform::rotate(float angle, const gfxm::vec3& axis)
{
    rotate(gfxm::angle_axis(angle, axis));
}

void Transform::rotate(const gfxm::quat& q)
{
    _rotation = 
        gfxm::normalize(
            q * 
            _rotation
        );
    dirty();
}

void Transform::lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up_vec, float f) {
    gfxm::mat3 mat_rotation (1.0f);
    gfxm::vec3 forward = gfxm::normalize(tgt - position());
    gfxm::vec3 up = gfxm::normalize(up_vec);
    gfxm::vec3 right = gfxm::normalize(gfxm::cross(forward, up));

    mat_rotation[0] = right;
    mat_rotation[1] = up;
    mat_rotation[2] = -forward;

    gfxm::quat q = gfxm::to_quat(mat_rotation);

    rotation(gfxm::slerp(rotation(), q, f));
}

void Transform::lookDir_(const gfxm::vec3& dir, const gfxm::vec3& forward, const gfxm::vec3& up, float f)
{
    //lookAt(worldPosition() - dir, forward, up, f);
}

void Transform::lookAt_(const gfxm::vec3& target, const gfxm::vec3& forward, const gfxm::vec3& up, float f)
{
    dirty();
	f = std::max(-1.0f, std::min(f, 1.0f));
	
    Transform* trans = this;
    gfxm::mat4 mat = trans->getTransform();
    gfxm::vec3 pos = mat[3];
    
    gfxm::vec3 newFwdUnit = gfxm::normalize(target - pos);
    gfxm::vec3 rotAxis = gfxm::normalize(gfxm::cross(forward, newFwdUnit));
    
    gfxm::quat q;
    float dot = gfxm::dot(forward, newFwdUnit);
	
    const float eps = 0.01f;
    if(fabs(dot + 1.0f) <= eps)
    {
        q = gfxm::angle_axis(gfxm::pi * f, trans->up());
    }/*
    else if(fabs(dot - 1.0f) <= eps)
    {
        q = gfxm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }*/
    else
	{
        float rotAngle = acosf(std::max(-1.0f, std::min(dot, 1.0f))) * f;
        q = gfxm::angle_axis(rotAngle, rotAxis);
    }
    
    trans->rotate(q);
}

void Transform::position(float x, float y, float z)
{ position(gfxm::vec3(x, y, z)); }
void Transform::position(const gfxm::vec3& position)
{ _position = position; dirty(); }
void Transform::rotation(float x, float y, float z)
{ _rotation = gfxm::euler_to_quat(gfxm::vec3(x, y, z)); dirty(); }
void Transform::rotation(gfxm::vec3 euler)
{ _rotation = gfxm::euler_to_quat(gfxm::vec3(euler.x, euler.y, euler.z)); dirty(); }
void Transform::rotation(float x, float y, float z, float w)
{ rotation(gfxm::quat(x, y, z, w)); }
void Transform::rotation(const gfxm::quat& rotation)
{ _rotation = rotation; dirty(); }

void Transform::scale(float s)
{ scale(gfxm::vec3(s, s, s)); }
void Transform::scale(float x, float y, float z)
{ scale(gfxm::vec3(x, y, z)); }
void Transform::scale(const gfxm::vec3& s)
{ _scale = s; dirty(); }
void Transform::scaleIncrement(const gfxm::vec3& s)
{ _scale = _scale + s; dirty(); }

gfxm::vec3 Transform::worldPosition()
{
    return getTransform()[3];
}
const gfxm::vec3& Transform::position()
{ return _position; }
gfxm::quat Transform::worldRotation()
{
    gfxm::mat3 m3 = gfxm::to_mat3(getTransform());
    m3[0] /= gfxm::length(m3[0]);
    m3[1] /= gfxm::length(m3[1]);
    m3[2] /= gfxm::length(m3[2]);
    return gfxm::to_quat(m3);
}
const gfxm::quat& Transform::rotation()
{ return _rotation; }
gfxm::vec3 Transform::rotationEuler()
{
    return gfxm::to_euler(_rotation);
}
const gfxm::vec3& Transform::scale()
{ return _scale; }

gfxm::vec3 Transform::right()
{ return getTransform()[0]; }
gfxm::vec3 Transform::up()
{ return getTransform()[1]; }
gfxm::vec3 Transform::back()
{ return getTransform()[2]; }
gfxm::vec3 Transform::left()
{ return -right(); }
gfxm::vec3 Transform::down()
{ return -up(); }
gfxm::vec3 Transform::forward()
{ return -back(); }

gfxm::quat Transform::getParentRotation()
{
    if(parentTransform())
        return gfxm::normalize(parentTransform()->getParentRotation() * parentTransform()->rotation());
    else
        return gfxm::quat(0.0f, 0.0f, 0.0f, 1.0f);
}

void Transform::setTransform(gfxm::mat4 t)
{
    _position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
    gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
    _rotation = gfxm::to_quat(rotMat);
    gfxm::vec3 right = t[0];
    gfxm::vec3 up = t[1];
    gfxm::vec3 back = t[2];
    _scale = gfxm::vec3(right.length(), up.length(), back.length());
    dirty();
}

gfxm::mat4 Transform::getLocalTransform() const
{
    return 
        gfxm::translate(gfxm::mat4(1.0f), _position) * 
        gfxm::to_mat4(_rotation) * 
        gfxm::scale(gfxm::mat4(1.0f), _scale);
}

gfxm::mat4 Transform::getParentTransform()
{
    if(parentTransform())
        return parentTransform()->getTransform();
    else
        return gfxm::mat4(1.0f);
}

gfxm::mat4 Transform::getTransform()
{
    if(_dirty)
    {
        _dirty = false;
        gfxm::mat4 localTransform = getLocalTransform();           
        if(parentTransform())
            _transform = parentTransform()->getTransform() * localTransform;
        else
            _transform = localTransform;
    }
    return _transform;
}

gfxm::mat4 Transform::getTransformForRoot(Transform* root) {
    gfxm::mat4 tr = getTransform();
    if(root) {
        return gfxm::inverse(root->getTransform()) * tr;
    } else {
        return getTransform();
    }
}

Transform* Transform::parentTransform() { 
    return getObject()->getParent() ? getObject()->getParent()->get<Transform>() : 0; 
}