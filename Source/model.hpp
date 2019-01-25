#ifndef MODEL_HPP
#define MODEL_HPP

#include "component.hpp"
#include "resource/mesh.hpp"
#include "resource/material.hpp"
#include "resource/resource_factory.h"

inline void writeResource(std::ostream& out, std::shared_ptr<Resource> res, Scene* scene);

class Model : public Component {
    CLONEABLE
public:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;

    virtual void serialize(std::ostream& out) {
        if(mesh) {
            uint8_t null_flag = 1;
            out.write((char*)&null_flag, sizeof(null_flag));
            
            uint8_t storage_flag = (uint8_t)mesh->Storage();
            out.write((char*)&storage_flag, sizeof(storage_flag));
            if(mesh->Storage() == Resource::LOCAL) {
                uint32_t res_id = getObject()->getScene()->getLocalResourceId<Mesh>(mesh);
                out.write((char*)&res_id, sizeof(res_id));
            } else {
                std::string res_name = mesh->Name();
                uint32_t name_sz = res_name.size();
                out.write((char*)&name_sz, sizeof(name_sz));
                out.write(res_name.data(), res_name.size());
            }
        } else {
            uint8_t null_flag = 0;
            out.write((char*)&null_flag, sizeof(null_flag));
        }
    }
    virtual void deserialize(std::istream& in) {
        uint8_t null_flag = 0;
        in.read((char*)&null_flag, sizeof(null_flag));
        if(null_flag == 1) {
            uint8_t storage_flag = 0;
            in.read((char*)&storage_flag, sizeof(storage_flag));
            Resource::STORAGE storage = (Resource::STORAGE)storage_flag;
            if(storage == Resource::LOCAL) {
                uint32_t res_id = 0;
                in.read((char*)&res_id, sizeof(res_id));
                
                mesh = getObject()->getScene()->getLocalResource<Mesh>(res_id);
                if(!mesh) {
                    LOG("Could not find local mesh resource by id: " << res_id);
                }
            } else if(storage == Resource::GLOBAL) {
                uint32_t name_sz = 0;
                in.read((char*)&name_sz, sizeof(name_sz));
                if(name_sz) {
                    std::string res_name;
                    res_name.resize(name_sz);
                    in.read((char*)res_name.data(), name_sz);
                    
                    mesh = getResource<Mesh>(res_name);
                }
            }
        }
    }

    virtual void _editorGui() {
        ImGui::Text("Mesh "); ImGui::SameLine();
        if(ImGui::SmallButton("MESH_BUTTON")) {

        }
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
