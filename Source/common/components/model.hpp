#ifndef MODEL_HPP
#define MODEL_HPP

#include "component.hpp"

#include "../../common/resource/mesh.hpp"
#include "../../common/resource/material.hpp"

#include "../../common/util/imgui_helpers.hpp"

class Model : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    struct SkinData {
        std::vector<GameObject*> bone_nodes;
        std::vector<gfxm::mat4> bind_transforms;
    };
    struct Segment {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        std::shared_ptr<SkinData> skin_data;
    };

    ~Model();

    virtual void onCreate();

    virtual void copy(Attribute* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        Model* o = (Model*)other;
        for(size_t i = 0; i < o->segmentCount(); ++i) {
            auto& this_seg = getSegment(i);
            auto& other_seg = o->getSegment(i);
            this_seg.mesh = other_seg.mesh;
            this_seg.material = other_seg.material;

            if(!other_seg.skin_data) {
                continue;
            }
            this_seg.skin_data.reset(new SkinData());
            for(
                size_t j = 0; 
                j < other_seg.skin_data->bone_nodes.size() && j < other_seg.skin_data->bind_transforms.size(); 
                ++j
            ) {
                this_seg.skin_data->bone_nodes.emplace_back(
                    getOwner()->getRoot()->findObject(
                        other_seg.skin_data->bone_nodes[j]->getName()
                    )
                );
                this_seg.skin_data->bind_transforms.emplace_back(
                    other_seg.skin_data->bind_transforms[j]
                );
            }
        }
    }

    virtual bool buildAabb(gfxm::aabb& out) {
        gfxm::aabb r;
        if(segmentCount()) {
            auto& seg = getSegment(0);
            auto& m = seg.mesh;
            r = m->aabb;
            for(size_t i = 1; i < segmentCount(); ++i) {
                auto& seg = getSegment(i);
                auto& m = seg.mesh;
                
                gfxm::expand_aabb(r, m->aabb.from);
                gfxm::expand_aabb(r, m->aabb.to);
            }
            out = r;
            return true;
        } else {
            return false;
        }
    }

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

    virtual bool serialize(out_stream& out) {
        DataWriter w(&out);
        w.write<uint32_t>(segmentCount());
        for(size_t i = 0; i < segmentCount(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";
            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            w.write(mesh_name);
            w.write<uint64_t>(0); // reserved
            w.write(mat_name);

            if(getSegment(i).skin_data) {
                auto& skin_data = getSegment(i).skin_data;
                w.write<uint32_t>(skin_data->bone_nodes.size());
                for(size_t j = 0; j < skin_data->bone_nodes.size(); ++j) {
                    if(skin_data->bone_nodes[j]) {
                        w.write(skin_data->bone_nodes[j]->getName());
                    } else {
                        w.write(std::string(""));
                    }
                    w.write<gfxm::mat4>(skin_data->bind_transforms[j]);
                }
            } else {
                w.write<uint32_t>(0);
            }
        }
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        DataReader r(&in);
        uint32_t seg_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < seg_count; ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            mesh_name = r.readStr();
            uint64_t reserved = r.read<uint64_t>();
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
                    std::string bone_name = r.readStr();
                    gfxm::mat4 t = r.read<gfxm::mat4>();
                    GameObject* bo = getOwner()->getRoot()->findObject(bone_name);
                    getSegment(i).skin_data->bone_nodes.emplace_back(bo);
                    getSegment(i).skin_data->bind_transforms.emplace_back(t);
                }
            }
        }
        return true;
    }

    virtual void onGui() {
        for(size_t i = 0; i < segmentCount(); ++i) {
            auto& seg = getSegment(i);
            ImGui::Text(MKSTR("Segment " << i).c_str());
            imguiResourceTreeCombo(MKSTR("mesh##" << i).c_str(), seg.mesh, "msh", [this](){
                LOG("Mesh changed");
            });
            imguiResourceTreeCombo(MKSTR("material##" << i).c_str(), seg.material, "mat", [this](){
                LOG("Material changed");
            });
            if(ImGui::SmallButton(MKSTR("- remove segment##" << i).c_str())) {
                removeSegment(i);
            }
        }
        if(ImGui::Button("+ Add segment")) {
            getSegment(segmentCount());
        }
    }

    virtual const char* getIconCode() const { return ICON_MDI_CUBE; }
private:
/* TODO: This needs to work
    virtual void onObjectRemoved(GameObject* sender) {
        for(size_t i = 0; i < segmentCount(); ++i) {
            auto& seg = getSegment(i);
            if(seg.skin_data) {
                for(size_t j = 0; j < seg.skin_data->bone_nodes.size(); ++j) {
                    if(sender == seg.skin_data->bone_nodes[j]) {
                        seg.skin_data->bone_nodes[j] = 0;
                    }
                }
            }
        }
    } */

    std::vector<Segment> segments;
};
STATIC_RUN(Model) {
    rttr::registration::class_<Model>("Model")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
