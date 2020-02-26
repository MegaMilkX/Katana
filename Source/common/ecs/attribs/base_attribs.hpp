#ifndef ECS_BASE_ATTRIBS_HPP
#define ECS_BASE_ATTRIBS_HPP

#include "../common/resource/mesh.hpp"
#include "../common/util/imgui_helpers.hpp"
#include <btBulletCollisionCommon.h>


#include "../common/ecs/attribs/transform.hpp"
#include "../common/ecs/attribs/transform_tree.hpp"

#include "../common/ecs/attribs/scene_graph_attribs.hpp"

#include "kt_cmd.hpp"

class ecsSubScene : public ecsAttrib<ecsSubScene> {
    std::shared_ptr<ecsWorld> world;
public:
    ecsSubScene() {}
    ecsSubScene(std::shared_ptr<ecsWorld> world)
    : world(world) {}
    ecsWorld* getWorld() const { return world.get(); }
    void onGui(ecsWorld* world, entity_id ent) override {
        if(ImGui::Button(ICON_MDI_PENCIL " Edit", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
            kt_cmd(MKSTR("ecs_world_subdoc " << (uint64_t)this->world.get()).c_str());
        }
    }

    void write(out_stream& out) override {
        world->serialize(out);
    }
    void read(in_stream& in) override {
        world.reset(new ecsWorld());
        world->deserialize(in, in.bytes_available());
    }
};

class ecsTagSubSceneRender : public ecsAttrib<ecsTagSubSceneRender> {};

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

    void write(out_stream& out) override {
        DataWriter w(&out);
        w.write(name);
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        name = r.readStr();
    }
};

class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        ImGui::DragFloat3("velo", (float*)&velo, 0.01f);
    }

    void write(out_stream& out) override {
        out.write(velo);
    }
    void read(in_stream& in) override {
        velo = in.read<gfxm::vec3>();
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

    void write(out_stream& out) override {
        out.write(mass);
    }
    void read(in_stream& in) override {
        mass = in.read<float>();
    }
};


#include "../../attributes/collision_shapes.hpp"
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
public:
    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));
    }
    std::shared_ptr<btCollisionShape> shape;

    void write(out_stream& out) override {
        
    }
    void read(in_stream& in) override {
        
    }
};
class ecsMeshes : public ecsAttrib<ecsMeshes> {
public:
    struct SkinData {
        std::vector<ecsWorldTransform*> bone_nodes;
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

    void write(out_stream& out) override {
        DataWriter w(&out);

        w.write<uint32_t>(segments.size());
        for(size_t i = 0; i < segments.size(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            w.write(mesh_name);
            w.write<uint8_t>(getSegment(i).submesh_index);
            w.write<uint8_t>(0); // reserved
            w.write<uint16_t>(0);
            w.write<uint32_t>(0); // ~
            w.write(mat_name);

            if(getSegment(i).skin_data) {
                auto& skin_data = getSegment(i).skin_data;
                w.write<uint32_t>(skin_data->bone_nodes.size());
                for(size_t j = 0; j < skin_data->bone_nodes.size(); ++j) {
                    // TODO:
                    //w.write(skin_data->bone_nodes[j]);
                    w.write<gfxm::mat4>(skin_data->bind_transforms[j]);
                }
            } else {
                w.write<uint32_t>(0);
            }
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);

        uint32_t seg_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < seg_count; ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            mesh_name = r.readStr();
            getSegment(i).submesh_index = (uint8_t)r.read<uint8_t>();
            r.read<uint8_t>(); // reserved
            r.read<uint16_t>();
            r.read<uint32_t>(); // ~
            mat_name = r.readStr();

            if(!mesh_name.empty()) {
                getSegment(i).mesh = retrieve<Mesh>(mesh_name);
            }
            if(!mat_name.empty()) {
                getSegment(i).material = retrieve<Material>(mat_name);
            }

            uint32_t bone_count = r.read<uint32_t>();
            if(bone_count) {
                getSegment(i).skin_data.reset(new SkinData());
                for(uint32_t j = 0; j < bone_count; ++j) {
                    gfxm::mat4 m = r.read<gfxm::mat4>();
                    getSegment(i).skin_data->bone_nodes.emplace_back((ecsWorldTransform*)0);
                    getSegment(i).skin_data->bind_transforms.emplace_back(m);
                }
            }
        }
    }
};


#endif
