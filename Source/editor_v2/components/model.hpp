#ifndef MODEL_HPP
#define MODEL_HPP

#include "component.hpp"

#include "../../common/resource/mesh.hpp"
#include "../../common/resource/material.hpp"

#include "../scene/game_scene.hpp"

#include "../../common/util/serialization.hpp"

class CmModel : public ObjectComponent, public SceneListener {
    RTTR_ENABLE(ObjectComponent)
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

    ~CmModel() {
        getOwner()->getScene()->getEventMgr().unsubscribeAll(this);
    }

    virtual void onCreate() {
        getOwner()->getScene()->getEventMgr().subscribe(this, EVT_OBJECT_REMOVED);
    }

    virtual void copy(ObjectComponent* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        CmModel* o = (CmModel*)other;
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
    size_t segmentCount() const {
        return segments.size();
    }

    virtual bool serialize(std::ostream& out) {
        write<uint32_t>(out, segmentCount());
        for(size_t i = 0; i < segmentCount(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";
            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            wt_string(out, mesh_name);
            write<uint64_t>(out, 0); // Reserved
            wt_string(out, mat_name);

            if(getSegment(i).skin_data) {
                auto& skin_data = getSegment(i).skin_data;
                write<uint32_t>(out, skin_data->bone_nodes.size());
                for(size_t j = 0; j < skin_data->bone_nodes.size(); ++j) {
                    if(skin_data->bone_nodes[j]) {
                        wt_string(out, skin_data->bone_nodes[j]->getName());
                    } else {
                        wt_string(out, "");
                    }
                    write<gfxm::mat4>(out, skin_data->bind_transforms[j]);
                }
            } else {
                write<uint32_t>(out, 0);
            }
        }
        return true;
    }
    virtual bool deserialize(std::istream& in, size_t sz) {
        uint32_t seg_count = read<uint32_t>(in);
        for(uint32_t i = 0; i < seg_count; ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            mesh_name = rd_string(in);
            uint64_t reserved = read<uint64_t>(in);
            mat_name = rd_string(in);
            if(!mesh_name.empty()) {
                getSegment(i).mesh = getResource<Mesh>(mesh_name);
            }
            if(!mat_name.empty()) {
                getSegment(i).material = getResource<Material>(mat_name);
            }

            uint32_t bone_count = read<uint32_t>(in);
            if(bone_count) {
                getSegment(i).skin_data.reset(new SkinData());
                for(uint32_t j = 0; j < bone_count; ++j) {
                    std::string bone_name = rd_string(in);
                    gfxm::mat4 t = read<gfxm::mat4>(in);
                    GameObject* bo = getOwner()->getRoot()->findObject(bone_name);
                    getSegment(i).skin_data->bone_nodes.emplace_back(bo);
                    getSegment(i).skin_data->bind_transforms.emplace_back(t);
                }
            }
        }
        return true;
    }
private:
    void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) {
        switch(e) {
        case EVT_OBJECT_REMOVED:
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
            break;
        };
    }

    std::vector<Segment> segments;
};
STATIC_RUN(CmModel) {
    rttr::registration::class_<CmModel>("CmModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
