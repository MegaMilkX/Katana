#ifndef MODEL_HPP
#define MODEL_HPP

#include "renderable_base.hpp"

#include "../../common/resource/mesh.hpp"
#include "../../common/resource/material.hpp"

#include "../../common/util/imgui_helpers.hpp"

class Model : public RenderableBase {
    RTTR_ENABLE(RenderableBase)
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

    ~Model();

    void addToDrawList(DrawList& dl) override {
        if(!getOwner()->isEnabled()) return;

        for(auto& seg : segments) {
            if(!seg.mesh) continue;
            
            GLuint vao = seg.mesh->mesh.getVao();
            Material* mat = seg.material.get();
            gfxm::mat4 transform = getOwner()->getTransform()->getWorldTransform();
            size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
            size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
            
            if(!seg.skin_data) {    // Static mesh
                DrawCmdSolid s;
                s.vao = vao;
                s.material = mat;
                s.indexCount = indexCount;
                s.indexOffset = indexOffset;
                s.transform = transform;
                s.object_ptr = getOwner();
                dl.solids.emplace_back(s);
            } else {                // Skinned mesh
                std::vector<gfxm::mat4> bone_transforms;
                for(auto t : seg.skin_data->bone_nodes) {
                    if(t) {
                        bone_transforms.emplace_back(t->getTransform()->getWorldTransform());
                    } else {
                        bone_transforms.emplace_back(gfxm::mat4(1.0f));
                    }
                }
                /*
                DrawCmdSkin s;
                s.vao = vao;
                s.material = mat;
                s.indexCount = indexCount;
                s.indexOffset = indexOffset;
                s.transform = transform;
                s.bone_transforms = bone_transforms;
                s.bind_transforms = seg.skin_data->bind_transforms;
                s.object_ptr = getOwner();
                dl.skins.emplace_back(s);*/
            }
        }
    }

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
        bool has_aabb = false;
        if(segmentCount()) {
            auto& seg = getSegment(0);
            auto& m = seg.mesh;
            if (m) {
                r = m->aabb;
                has_aabb = true;
            }
            for(size_t i = 1; i < segmentCount(); ++i) {
                auto& seg = getSegment(i);
                auto& m = seg.mesh;
                if (!m) continue;
                has_aabb = true;
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

    void write(SceneWriteCtx& o) {
        o.write<uint32_t>(segmentCount());
        for(size_t i = 0; i < segmentCount(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";
            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            o.write(mesh_name);
            o.write<uint8_t>(getSegment(i).submesh_index);
            o.write<uint8_t>(0); // reserved
            o.write<uint16_t>(0);
            o.write<uint32_t>(0); // ~
            o.write(mat_name);

            if(getSegment(i).skin_data) {
                auto& skin_data = getSegment(i).skin_data;
                o.write<uint32_t>(skin_data->bone_nodes.size());
                for(size_t j = 0; j < skin_data->bone_nodes.size(); ++j) {
                    o.write(skin_data->bone_nodes[j]);
                    o.write<gfxm::mat4>(skin_data->bind_transforms[j]);
                }
            } else {
                o.write<uint32_t>(0);
            }
        }
    }
    void read(SceneReadCtx& in) {
        uint32_t seg_count = in.read<uint32_t>();
        for(uint32_t i = 0; i < seg_count; ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            mesh_name = in.readStr();
            getSegment(i).submesh_index = (uint8_t)in.read<uint8_t>();
            in.read<uint8_t>(); // reserved
            in.read<uint16_t>();
            in.read<uint32_t>(); // ~
            mat_name = in.readStr();
            if(!mesh_name.empty()) {
                getSegment(i).mesh = retrieve<Mesh>(mesh_name);
            }
            if(!mat_name.empty()) {
                getSegment(i).material = retrieve<Material>(mat_name);
            }

            uint32_t bone_count = in.read<uint32_t>();
            if(bone_count) {
                getSegment(i).skin_data.reset(new SkinData());
                for(uint32_t j = 0; j < bone_count; ++j) {
                    ktNode* bo = in.readNode();
                    gfxm::mat4 t = in.read<gfxm::mat4>();
                    getSegment(i).skin_data->bone_nodes.emplace_back(bo);
                    getSegment(i).skin_data->bind_transforms.emplace_back(t);
                }
            }
        }
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
            w.write<uint8_t>(getSegment(i).submesh_index);
            w.write<uint8_t>(0); // reserved
            w.write<uint16_t>(0);
            w.write<uint32_t>(0); // ~
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
                    std::string bone_name = r.readStr();
                    gfxm::mat4 t = r.read<gfxm::mat4>();
                    ktNode* bo = getOwner()->getRoot()->findObject(bone_name);
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
        if(ImGui::Button(ICON_MDI_PLUS " Add segment")) {
            getSegment(segmentCount());
        }
    }

    virtual const char* getIconCode() const { return ICON_MDI_CUBE; }
private:
/* TODO: This needs to work
    virtual void onObjectRemoved(ktNode* sender) {
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

#endif
