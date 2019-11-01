#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"


class ecsSceneNodeArch : public ecsArchetype<ecsTransform> {
public:
};

class ecsysSceneGraph : public ecsSystem<
    ecsSceneNodeArch
> {
    //std::set<ecsSceneNodeArch*> root_nodes;
    std::set<entity_id> root_entities;

    void onFit(ecsSceneNodeArch* object) override {
        object->get<ecsTransform>()->entity_uid = object->getEntityUid();
        //root_nodes.insert(object);
    }
    void onUnfit(ecsSceneNodeArch* object) override {
        //root_nodes.erase(object);
    }

    void onUpdate() {
        
    }

    void imguiTreeNode(ecsWorld* world, entity_id e, entity_id* selected) {
        ecsName* name_attrib = getWorld()->findAttrib<ecsName>(e);
        ecsTransform* node = getWorld()->findAttrib<ecsTransform>(e);
        std::string entity_name = "[anonymous]";
        if(name_attrib) {
            if(name_attrib->name.size()) {
                entity_name = name_attrib->name;
            } else {
                entity_name = "[empty_name]";
            }
        }
        if(node && node->_children.size()) {
            ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
            if(selected && (*selected == e)) {
                tree_node_flags |= ImGuiTreeNodeFlags_Selected;
            }/*
            if(node == gResourceTree.getRoot()) {
                tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }*/
            bool open = ImGui::TreeNodeEx(MKSTR(entity_name << "###" << e).c_str(), tree_node_flags);
            if(selected && ImGui::IsItemClicked(0)) {
                *selected = e;
            }
            if(open) {
                for(auto& c : node->_children) {
                    imguiTreeNode(world, c->entity_uid, selected);
                }

                ImGui::TreePop();
            }
        } else {
            if(ImGui::Selectable(MKSTR(entity_name << "###" << e).c_str(), selected && (*selected == e))) {
                if(selected) *selected = e;
            }
        }
    }

public:
    entity_id createNode() {
        entity_id ent = world->createEntity(ecsSceneNodeArch::get_inclusion_sig());
        root_entities.insert(ent);
        return ent;
    }
    void setParent(entity_id child, entity_id parent) {
        world->getAttrib<ecsTransform>(child)->setParent(
            world->getAttrib<ecsTransform>(parent)
        );
        root_entities.erase(child);
    }
    void removeParent(entity_id child) {
        world->getAttrib<ecsTransform>(child)->setParent(0);
        root_entities.insert(child);
    }

    void onGui(entity_id* selected = 0) {
        for(auto e : root_entities) {
            imguiTreeNode(getWorld(), e, selected);
        }
    }
};


#endif
