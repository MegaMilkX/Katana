#ifndef MODEL_HPP
#define MODEL_HPP

#include "component.hpp"
#include "resource/mesh.hpp"
#include "resource/material.hpp"
#include "resource/resource_factory.h"

#include "util/serialization.hpp"

inline void writeResource(std::ostream& out, std::shared_ptr<Resource> res, Scene* scene);

class Model : public Component {
    CLONEABLE_AUTO
public:
    struct Segment {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
    };

    std::vector<Segment> segments;

    Segment& getSegment(size_t i) {
        if(i >= segments.size()) {
            segments.resize(i + 1);
        }
        return segments[i];
    }

    size_t segmentCount() const {
        return segments.size();
    }

    virtual void serialize(std::ostream& out) {
        write<uint32_t>(out, segmentCount());
        for(size_t i = 0; i < segmentCount(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";
            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            wt_string(out, mesh_name);
            write<uint64_t>(out, 0); // Reserved
            wt_string(out, mat_name);
        }
    }
    virtual void deserialize(std::istream& in, size_t sz) {
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
        }
    }

    virtual void _editorGui() {
        /*
        ImGui::Text("Mesh "); ImGui::SameLine();
        std::string mesh_name = "! No mesh !";
        if(mesh) {
            mesh_name = mesh->Name();
        }
        if(ImGui::SmallButton(mesh_name.c_str())) {

        }
        ImGui::PushID(mesh_name.c_str());
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                //SceneObject* tgt_dnd_so = *(SceneObject**)payload->Data;
                //tgt_dnd_so->setParent(so);
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                mesh = getResource<Mesh>(fname);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();

        ImGui::Text("Material "); ImGui::SameLine();
        std::string mat_name = "! No material !";
        if(material) {
            mat_name = material->Name();
        }
        if(ImGui::SmallButton(mat_name.c_str())) {

        }
        ImGui::PushID(mat_name.c_str());
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                //SceneObject* tgt_dnd_so = *(SceneObject**)payload->Data;
                //tgt_dnd_so->setParent(so);
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                material = getResource<Material>(fname);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
        */
    }
};
STATIC_RUN(Model)
{
    rttr::registration::class_<Model>("Model")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
