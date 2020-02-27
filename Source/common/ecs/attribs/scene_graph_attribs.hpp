#ifndef SCENE_GRAPH_ATTRIBS_HPP
#define SCENE_GRAPH_ATTRIBS_HPP


#include "../attribute.hpp"

#include "../../lib/imguizmo/ImGuizmo.h"

class ecsysSceneGraph;
class tupleTransform;
class ecsTranslation : public ecsAttrib<ecsTranslation> {
    friend ecsysSceneGraph;
    friend tupleTransform;
    ecsysSceneGraph* system = 0;
    size_t dirty_index = 0;
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
    friend ecsysSceneGraph;
    friend tupleTransform;
    ecsysSceneGraph* system = 0;
    size_t dirty_index = 0;
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
    friend ecsysSceneGraph;
    friend tupleTransform;
    ecsysSceneGraph* system = 0;
    size_t dirty_index = 0;
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

class ecsTRS : public ecsAttrib<ecsTRS> {
    friend ecsysSceneGraph;
    friend tupleTransform;
    ecsysSceneGraph* system = 0;
    size_t dirty_index = 0;
    gfxm::vec3 position = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
    gfxm::vec3 scale = gfxm::vec3(1.0f, 1.0f, 1.0f);

public:
    void dirty();

    
    void rotate(float angle, float axisX, float axisY, float axisZ);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    
    void setRotation(float x, float y, float z);
    void setRotation(gfxm::vec3 euler);
    void setRotation(const gfxm::quat& rotation);
    void setRotation(float x, float y, float z, float w);
    void setScale(float s);
    void setScale(float x, float y, float z);
    void setScale(const gfxm::vec3& s);
    void setTransform(gfxm::mat4 t);

    
    const gfxm::quat& getRotation() const;
    const gfxm::vec3& getScale() const;

    void fromMatrix(const gfxm::mat4 t) {
        position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
        gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
        rotation = gfxm::to_quat(rotMat);
        gfxm::vec3 right = t[0];
        gfxm::vec3 up = t[1];
        gfxm::vec3 back = t[2];
        scale = gfxm::vec3(right.length(), up.length(), back.length());
        dirty();
    }


    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat3("Translation", (float*)&position, 0.001f)) {
            dirty();
        }
        if(ImGui::DragFloat4("Quaternion", (float*)&rotation, 0.001f)) {
            dirty();
        }
        if(ImGui::DragFloat3("Scale", (float*)&scale, 0.001f)) {
            dirty();
        }
    }
};

class ecsWorldTransform : public ecsAttrib<ecsWorldTransform> {
public:
    gfxm::mat4 transform = gfxm::mat4(1.0f);
};

class ecsParentTransform : public ecsAttrib<ecsParentTransform> {
public:
    entity_id          parent_entity;
    ecsWorldTransform* parent_transform = 0;

    void write(ecsWorldWriteCtx& out) override {
        out.writeEntityRef(parent_entity);
        out.writeAttribRef(parent_transform);
    }
    void read(ecsWorldReadCtx& in) override {
        parent_entity = in.readEntityRef();
        parent_transform = (ecsWorldTransform*)in.readAttribRef();
    }
};


#endif
