#ifndef SCENE_GRAPH_ATTRIBS_HPP
#define SCENE_GRAPH_ATTRIBS_HPP


#include "../attribute.hpp"

#include "../../lib/imguizmo/ImGuizmo.h"


class ecsTranslation : public ecsAttrib<ecsTranslation> {
    gfxm::vec3 position = gfxm::vec3(.0f, .0f, .0f);
public:
    void dirty();

    void translate(float x, float y, float z);
    void translate(const gfxm::vec3& vec);

    void setPosition(float x, float y, float z);
    void setPosition(const gfxm::vec3& position);

    const gfxm::vec3& getPosition() const;

    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat3("Translation", (float*)&position, 0.001f)) {
            dirty();
        }
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write(position);
    }
    void read(ecsWorldReadCtx& in) override {
        position = in.read<gfxm::vec3>();
    }
};

class ecsRotation : public ecsAttrib<ecsRotation> {
    gfxm::quat _rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
public:
    void dirty();

    void rotate(float angle, float axisX, float axisY, float axisZ);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    void setRotation(float x, float y, float z);
    void setRotation(gfxm::vec3 euler);
    void setRotation(const gfxm::quat& rotation);
    void setRotation(float x, float y, float z, float w);

    const gfxm::quat& getRotation() const;

    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat4("Quaternion", (float*)&_rotation, 0.001f)) {
            dirty();
        }
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write(_rotation);
    }
    void read(ecsWorldReadCtx& in) override {
        _rotation = in.read<gfxm::quat>();
    }
};

class ecsScale : public ecsAttrib<ecsScale> {
    gfxm::vec3 _scale = gfxm::vec3(1.f, 1.f, 1.f);
public:
    void dirty();

    void setScale(float s);
    void setScale(float x, float y, float z);
    void setScale(const gfxm::vec3& s);

    const gfxm::vec3& getScale() const;

    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat3("Scale", (float*)&_scale, 0.001f)) {
            dirty();
        }
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write(_scale);
    }
    void read(ecsWorldReadCtx& in) override {
        _scale = in.read<gfxm::vec3>();
    }
};

class ecsWorldTransform : public ecsAttrib<ecsWorldTransform> {
    gfxm::mat4 transform = gfxm::mat4(1.0f);
public:
    void setTransform(const gfxm::mat4& t) {
        transform = t;
        getEntityHdl().signalUpdate<ecsWorldTransform>();
    }
    const gfxm::mat4& getTransform() const {
        return transform;
    }

    gfxm::vec3 getPosition() const {
        return (gfxm::vec3)(transform * gfxm::vec4(0, 0, 0, 1));
    }
};


#endif
