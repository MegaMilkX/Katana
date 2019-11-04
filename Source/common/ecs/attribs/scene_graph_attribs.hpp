#ifndef SCENE_GRAPH_ATTRIBS_HPP
#define SCENE_GRAPH_ATTRIBS_HPP


#include "../attribute.hpp"


class ecsysSceneGraph;
class ecsTRS : public ecsAttrib<ecsTRS> {
public:
    ecsysSceneGraph* system = 0;
    size_t dirty_index = 0;
    gfxm::vec3 position = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
    gfxm::vec3 scale = gfxm::vec3(1.0f, 1.0f, 1.0f);

    void dirty();

    void fromMatrix(const gfxm::mat4 t) {
        position = gfxm::vec3(t[3].x, t[3].y, t[3].z);
        gfxm::mat3 rotMat = gfxm::to_orient_mat3(t);
        rotation = gfxm::to_quat(rotMat);
        gfxm::vec3 right = t[0];
        gfxm::vec3 up = t[1];
        gfxm::vec3 back = t[2];
        scale = gfxm::vec3(right.length(), up.length(), back.length());
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
};


#endif
