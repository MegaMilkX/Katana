#include "scene_object.hpp"
#include "scene.hpp"

#include "lib/imgui_wrap.hpp"

void SceneObject::destroy() {
    if(!scene) return;
    scene->removeObject(this);
}

size_t SceneObject::getId() const {
    return id;
}

void SceneObject::setName(const std::string& name) {
    this->name = name;
}
std::string SceneObject::getName() const {
    return name;
}

Component* SceneObject::get(const std::string& type_name) {
    if(!scene) return 0;
    rttr::type type = rttr::type::get_by_name(type_name);
    if(!type.is_valid()) {
        LOG_WARN(type_name << " is not a valid type");
        return 0;
    }
    Component* c = find(type_name);
    if(!c) {
        c = getScene()->createComponent(type_name, this);
        if(c) {
            components[type] = c;
        }
    } 
    return c;
}

Component* SceneObject::find(const std::string& type_name) {
    if(!scene) return 0;
    rttr::type type = rttr::type::get_by_name(type_name);
    if(!type.is_valid()) {
        LOG_WARN(type_name << " is not a valid type");
        return 0;
    }
    if(components.count(type) == 0) {
        return 0;
    }
    return components[type];
}

void SceneObject::removeComponent(rttr::type t) {
    scene->removeComponent(t, components[t]);
    components.erase(t);
}

void SceneObject::removeComponents() {
    if(!scene) return;
    for(auto kv : components) {
        scene->removeComponent(kv.first, kv.second);
    }
    components.clear();
}

SceneObject* SceneObject::createChild() {
    if(!scene) return 0;
    SceneObject* so = scene->createObject(this);
    return so;
}

size_t SceneObject::childCount() {
    return children.size();
}
SceneObject* SceneObject::getChild(size_t index) {
    return children[index];
}
void SceneObject::removeChild(SceneObject* so) {
    for(size_t i = 0; i < children.size(); ++i) {
        if(children[i] == so) {
            children.erase(children.begin() + i);
            return;
        }
    }
}

SceneObject* SceneObject::getParent() {
    return parent;
}
void SceneObject::setParent(SceneObject* so) {
    if(id == 0) {
        return;
    }
    
    if(parent) {
        for(size_t i = 0; i < parent->children.size(); ++i) {
            if(parent->children[i] == this) {
                parent->children.erase(parent->children.begin() + i);
                break;
            }
        }
    }
    parent = so;
    if(parent) {
        parent->children.emplace_back(this);
    }
}

void SceneObject::cloneFrom(SceneObject* other) {
    if(!scene) return;
    removeComponents();

    std::map<Component*, Component*> copy_pairs;
    for(auto kv : other->components) {
        Component* c = scene->createComponentCopy(kv.first, kv.second, this);
        if(c) {
            components[kv.first] = c;
            copy_pairs[c] = kv.second;
        } else {
            LOG_WARN("Failed to clone " << kv.first.get_name().to_string() << " component (clone() not implemented)");
        }
    }

    for(auto& kv : copy_pairs) {
        kv.first->onCreate();
        kv.first->_onClone(kv.second);
    }

    for(auto& kv : components) {
        getScene()->triggerProbeOnCreateRecursive(kv.first, kv.second);
    }
}

SceneObject* SceneObject::findObject(const std::string& name) {
    if(this->name == name) {
        return this;
    }
    
    for(auto o : children) {
        SceneObject* result = 0;
        if(result = o->findObject(name)) {
            return result;
        }
    }
    return 0;
}

Scene* SceneObject::getScene() {
    return scene;
}

void SceneObject::serialize(std::ostream& out) {
    uint32_t parent_index = 0;
    if(getParent()) {
        parent_index = (uint32_t)getParent()->getId();
    }
    out.write((char*)&parent_index, sizeof(parent_index));

    uint32_t name_len = (uint32_t)name.size();
    out.write((char*)&name_len, sizeof(name_len));
    out.write(name.data(), name.size());
    
    uint32_t child_count = (uint32_t)childCount();
    out.write((char*)&child_count, sizeof(child_count));

    for(size_t i = 0; i < childCount(); ++i) {
        SceneObject* ch = getChild(i);
        uint32_t ch_id = ch->getId();
        out.write((char*)&ch_id, sizeof(child_count));
    }
}

void SceneObject::deserialize(std::istream& in) {
    uint32_t parent_id = 0;
    in.read((char*)&parent_id, sizeof(parent_id));

    uint32_t name_len = 0;
    in.read((char*)&name_len, sizeof(name_len));
    name.resize(name_len);
    in.read((char*)name.data(), name_len);

    uint32_t child_count = 0;
    in.read((char*)&child_count, sizeof(child_count));

    this->parent = getScene()->getObject(parent_id);
    this->children.resize(child_count);

    for(uint32_t i = 0; i < child_count; ++i) {
        uint32_t ch_id = 0;
        in.read((char*)&ch_id, sizeof(ch_id));
        children[i] = getScene()->getObject(ch_id);
    }
}

#include "model.hpp"
#include "transform.hpp"
#include "components/collider.hpp"
#include "light.hpp"
#include "animator.hpp"
#include "character.hpp"
#include "camera.hpp"
#include "tps_camera.hpp"

void SceneObject::_editorGui() {
    static SceneObject* component_creator_so_tgt = 0;
    if(ImGui::Button("Add component...")) {
        ImGui::OpenPopup("ComponentCreator");
        component_creator_so_tgt = this;
    }
    bool dummy_open = true;
    if (ImGui::BeginPopupModal("ComponentCreator", &dummy_open))
    {
        ImGui::BeginChild("ComponentList", ImVec2(150, 100), false, 0);
        bool selected = false;
        if(ImGui::Selectable("Collider", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Collider>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Animator", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Animator>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Model", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Model>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Transform", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Transform>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("LightOmni", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<LightOmni>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Character", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Character>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Camera", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<Camera>();
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::Selectable("Third person camera", &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                component_creator_so_tgt->get<TpsCamera>();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    for(auto kv : components) {
        bool exists = true;
        if(ImGui::CollapsingHeader(kv.first.get_name().to_string().c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
            kv.second->_editorGui();
        }
        if(!exists) {
            removeComponent(kv.first);
        }
    }
}