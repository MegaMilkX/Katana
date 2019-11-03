#ifndef ECS_BASE_ATTRIBS_HPP
#define ECS_BASE_ATTRIBS_HPP

#include "../common/resource/mesh.hpp"
#include "../common/util/imgui_helpers.hpp"
#include <btBulletCollisionCommon.h>


#include "../common/ecs/attribs/transform.hpp"
#include "../common/ecs/attribs/transform_tree.hpp"

class ecsTRS : public ecsAttrib<ecsTRS> {
public:
    gfxm::vec3 position = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
    gfxm::vec3 scale = gfxm::vec3(1.0f, 1.0f, 1.0f);

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
        if(ImGui::DragFloat3("Translation", (float*)&position, 0.001f)) {}
        if(ImGui::DragFloat4("Quaternion", (float*)&rotation, 0.001f)) {}
        if(ImGui::DragFloat3("Scale", (float*)&scale, 0.001f)) {}
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
class ecsTransformChildren : public ecsAttrib<ecsTransformChildren> {
public:
    std::set<entity_id> entities;
};

class ecsName : public ecsAttrib<ecsName> {
public:
    std::string name;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, name.c_str(), name.size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            name = buf;
        }
    }
};

class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        ImGui::DragFloat3("velo", (float*)&velo, 0.01f);
    }
};
class ecsMass : public ecsAttrib<ecsMass> {
public:
    float mass = 1.0f;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat("mass", &mass, 0.01f)) {
            world->updateAttrib(ent, *this);
        }
    }
};
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
public:
    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));
    }
    std::shared_ptr<btCollisionShape> shape;
};
class ecsMeshes : public ecsAttrib<ecsMeshes> {
public:
    struct SkinData {
        std::vector<ktNode*> bone_nodes;
        std::vector<gfxm::mat4> bind_transforms;
    };
    struct Segment {
        std::shared_ptr<Mesh> mesh;
        uint8_t               submesh_index;
        std::shared_ptr<Material> material;
        std::shared_ptr<SkinData> skin_data;
    };

    std::vector<Segment> segments;

    Segment& getSegment(size_t i) {
        if(i >= segments.size()) {
            segments.resize(i + 1);
        }
        return segments[i];
    }
    void removeSegment(size_t i) {
        segments.erase(segments.begin() + i);
    }
    size_t segmentCount() const {
        return segments.size();
    }

    virtual void onGui(ecsWorld* world, entity_id ent) {
        for(size_t i = 0; i < segmentCount(); ++i) {
            auto& seg = getSegment(i);

            if(ImGui::CollapsingHeader(MKSTR("Mesh segment " << i).c_str())) {
                ImGui::Text(MKSTR("Segment " << i).c_str());
                bool seg_removed = false;
                ImGui::SameLine();
                if(ImGui::SmallButton(ICON_MDI_DELETE)) {
                    seg_removed = true;
                }
                imguiResourceTreeCombo(MKSTR("mesh##" << i).c_str(), seg.mesh, "msh", [this](){
                    LOG("Mesh changed");
                });
                if(seg.mesh && (seg.mesh->submeshes.size() > 1)) {
                    int submesh_index = (int)seg.submesh_index;
                    if(ImGui::DragInt(MKSTR("submesh##" << i).c_str(), &submesh_index, 1.0f, 0, seg.mesh->submeshes.size() - 1)) {
                        seg.submesh_index = (uint8_t)submesh_index;
                    }
                }
                imguiResourceTreeCombo(MKSTR("material##" << i).c_str(), seg.material, "mat", [this](){
                    LOG("Material changed");
                });
                if(seg_removed) {
                    removeSegment(i);
                }
            }
        }
        if(ImGui::Button(ICON_MDI_PLUS " Add segment")) {
            getSegment(segmentCount());
        }
    }
};


#endif
