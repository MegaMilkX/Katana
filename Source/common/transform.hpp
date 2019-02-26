#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "component.hpp"
#include "gfxm.hpp"
#include <vector>

#include "scene_object.hpp"
#include "util/log.hpp"

class Transform : public Component
{
    CLONEABLE
    RTTR_ENABLE(Component)
public:
    Transform()
    : _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f, 1.0f),
    _scale(1.0f, 1.0f, 1.0f),
    _dirty(true) { }
    ~Transform() {
    }

    void onClone(Transform* other) {
        _position = other->_position;
        _rotation = other->_rotation;
        _scale = other->_scale;
        _dirty = other->_dirty;
        _transform = other->_transform;
    }

    void dirty();
    
    void translate(float x, float y, float z);
    void translate(const gfxm::vec3& vec);
    void rotate(float angle, float axisX, float axisY, float axisZ);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    void lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up, float f = 1.0f);
    void lookDir_(const gfxm::vec3& dir, const gfxm::vec3& forward, const gfxm::vec3& up, float f);
    void lookAt_(const gfxm::vec3& target, const gfxm::vec3& forward, const gfxm::vec3& up, float f);

    void position(float x, float y, float z);
    void position(const gfxm::vec3& position);
    void rotation(float x, float y, float z);
    void rotation(gfxm::vec3 euler);
    void rotation(const gfxm::quat& rotation);
    void rotation(float x, float y, float z, float w);
    void scale(float s);
    void scale(float x, float y, float z);
    void scale(const gfxm::vec3& s);
    void scaleIncrement(const gfxm::vec3& s);
    
    gfxm::vec3 worldPosition();
    const gfxm::vec3& position();
    gfxm::quat worldRotation();
    const gfxm::quat& rotation();
    gfxm::vec3 rotationEuler();
    gfxm::vec3 worldScale();
    const gfxm::vec3& scale();
    
    gfxm::vec3 right();
    gfxm::vec3 up();
    gfxm::vec3 back();
    gfxm::vec3 left();
    gfxm::vec3 down();
    gfxm::vec3 forward();
    
    gfxm::quat getParentRotation();

    void setTransform(gfxm::mat4 t);
    gfxm::mat4 getLocalTransform() const;
    gfxm::mat4 getParentTransform();
    gfxm::mat4 getTransform();
    gfxm::mat4 getTransformForRoot(Transform* root);

    void toWorldPosition(gfxm::vec3& pos)
    {
        gfxm::vec4 posw(pos.x, pos.y, pos.z, 1.0f);
        posw = (getTransform()) * posw;
        pos = gfxm::vec3(posw.x, posw.y, posw.z);
        //pos = gfxm::inverse(GetTransform()) * pos;
    }

    void toWorldDirection(gfxm::vec3& dir)
    {
        gfxm::mat3 m3 = gfxm::to_mat3(getParentTransform());
        dir = m3 * dir;
    }

    void toWorldRotation(gfxm::quat& q)
    {
        if(parentTransform())
        {
            q = gfxm::normalize(gfxm::inverse(worldRotation()) * q);
        }
    }

    void toWorldScale(gfxm::vec3& vec)
    {
        vec = gfxm::inverse(getTransform()) * vec;
    }
    
    Transform* parentTransform();

    virtual void _editorGui() {
        if(ImGui::DragFloat3("Translation", (float*)&_position, 0.001f)) {
            dirty();
        }
        gfxm::vec3 euler = gfxm::to_euler(_rotation);
        if(ImGui::DragFloat3("Rotation", (float*)&euler, 0.001f)) {
            _rotation = gfxm::to_quat(euler);
            dirty();
        }
        if(ImGui::DragFloat3("Scale", (float*)&_scale, 0.001f)) {
            dirty();
        }
    }
    // ====
    /*
    virtual bool _write(std::ostream& out, ExportData& exportData) {
        out.write((char*)&_position, sizeof(_position));
        out.write((char*)&_rotation, sizeof(_rotation));
        out.write((char*)&_scale, sizeof(_scale));
        return true;
    }
    virtual bool _read(std::istream& in, size_t sz, ImportData& importData) {
        if(sz != sizeof(_position) + sizeof(_rotation) + sizeof(_scale)) 
            return false;
        in.read((char*)&_position, sizeof(_position));
        in.read((char*)&_rotation, sizeof(_rotation));
        in.read((char*)&_scale, sizeof(_scale));
        if(Object()->Parent()) {
            Object()->Parent()->Get<Transform>()->Attach(this);
        }
        return true;
    }
    virtual bool _editor() {
        if(ImGui::DragFloat3("Translation", (float*)&_position, 0.001f)) {
            dirty();
        }
        gfxm::vec3 euler = gfxm::to_euler(_rotation);
        if(ImGui::DragFloat3("Rotation", (float*)&euler, 0.001f)) {
            _rotation = gfxm::to_quat(euler);
            dirty();
        }
        if(ImGui::DragFloat3("Scale", (float*)&_scale, 0.001f)) {
            dirty();
        }
        
        return true;
    }
    */

    virtual void serialize(std::ostream& out) {
        out.write((char*)&_position, sizeof(_position));
        out.write((char*)&_rotation, sizeof(_rotation));
        out.write((char*)&_scale, sizeof(_scale));
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        in.read((char*)&_position, sizeof(_position));
        in.read((char*)&_rotation, sizeof(_rotation));
        in.read((char*)&_scale, sizeof(_scale));
        dirty();
    }
private:
    bool _dirty;
    gfxm::vec3 _position;
    gfxm::quat _rotation;
    gfxm::vec3 _scale;
    gfxm::mat4 _transform;
};
STATIC_RUN(Transform)
{
    rttr::registration::class_<Transform>("Transform")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
