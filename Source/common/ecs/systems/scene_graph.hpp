#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

typedef ecsArchetype<ecsParentTransform, ecsWorldTransform> archGraphTransform;

class ecsysSceneGraph : public ecsSystem<
    ecsArchetype<ecsWorldTransform>,
    ecsArchetype<ecsParentTransform>,
    ecsArchetype<ecsTRS, ecsWorldTransform>,
    archGraphTransform
> {
    bool hierarchy_dirty = true;
    struct Node {
        entity_id id;
        Node* parent = 0;
        std::set<Node*> children;

        bool operator<(const Node& other) const {
            return id < other.id;
        }
    };
    std::map<entity_id, Node> nodes;
    std::set<Node*> root_nodes;

    std::vector<archGraphTransform*> dirty_vector;

    void onFit(ecsArchetype<ecsParentTransform>* o) {
        hierarchy_dirty = true;
    }
    void onUnfit(ecsArchetype<ecsParentTransform>* o) {
        hierarchy_dirty = true;
    }

    void onUnfit(ecsArchetype<ecsWorldTransform>* o) {
        for(auto& a : get_array<ecsArchetype<ecsParentTransform>>()) {
            if(a->get<ecsParentTransform>()->parent_entity == o->getEntityUid()) {
                world->removeAttrib(a->getEntityUid(), ecsParentTransform::get_id_static());
            }
        }
        hierarchy_dirty = true;
    }

    void onFit(archGraphTransform* a) {

    }
    void onUnfit(archGraphTransform* a) {

    }
    

    void onUpdate() {
        for(auto& a : get_array<ecsArchetype<ecsTRS, ecsWorldTransform>>()) {
            ecsTRS* trs = a->get<ecsTRS>();
            ecsWorldTransform* world = a->get<ecsWorldTransform>();

            world->transform = gfxm::translate(gfxm::mat4(1.0f), trs->position) * 
                            gfxm::to_mat4(trs->rotation) * 
                            gfxm::scale(gfxm::mat4(1.0f), trs->scale);
        }
        for(auto& a : get_array<archGraphTransform>()) {
            ecsWorldTransform* parent_world = a->get<ecsParentTransform>()->parent_transform;
            ecsWorldTransform* world = a->get<ecsWorldTransform>();

            if(parent_world) {
                world->transform = parent_world->transform * world->transform;
            }
        }
    }

    void imguiTreeNode(ecsWorld* world, Node* node, entity_id* selected) {
        ecsName* name_attrib = getWorld()->findAttrib<ecsName>(node->id);
        std::string entity_name = "[anonymous]";
        if(name_attrib) {
            if(name_attrib->name.size()) {
                entity_name = name_attrib->name;
            } else {
                entity_name = "[empty_name]";
            }
        }
        if(node->children.size()) {
            ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
            if(selected && (*selected == node->id)) {
                tree_node_flags |= ImGuiTreeNodeFlags_Selected;
            }/*
            if(node == gResourceTree.getRoot()) {
                tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }*/
            bool open = ImGui::TreeNodeEx(MKSTR(entity_name << "###" << node->id).c_str(), tree_node_flags);
            if(selected && ImGui::IsItemClicked(0)) {
                *selected = node->id;
            }
            if(open) {
                for(auto& n : node->children) {
                    imguiTreeNode(world, n, selected);
                }

                ImGui::TreePop();
            }
        } else {
            if(ImGui::Selectable(MKSTR(entity_name << "###" << node->id).c_str(), selected && (*selected == node->id))) {
                if(selected) *selected = node->id;
            }
        }
    }

public:
    entity_id createNode() {
        entity_id ent = world->createEntity();
        return ent;
    }
    void setParent(entity_id child, entity_id parent) {
        world->getAttrib<ecsParentTransform>(child)->parent_transform = 
            world->getAttrib<ecsWorldTransform>(parent);
        world->getAttrib<ecsParentTransform>(child)->parent_entity = parent;
        hierarchy_dirty = true;
    }
    void removeParent(entity_id child) {
        ecsParentTransform* parent_transform = world->findAttrib<ecsParentTransform>(child);
        if(!parent_transform) {
            return;
        }

        world->removeAttrib(child, ecsParentTransform::get_id_static());
        hierarchy_dirty = true;
    }

    void onGui(entity_id* selected = 0) {
        if(hierarchy_dirty) {
            root_nodes.clear();
            nodes.clear();
            for(auto e : world->getEntities()) {
                nodes[e].id = e;
            }
            for(auto e : world->getEntities()) {
                ecsParentTransform* parent = world->findAttrib<ecsParentTransform>(e);
                if(parent == 0) {
                    root_nodes.insert(&nodes[e]);
                } else {
                    nodes[e].parent = &nodes[parent->parent_entity];
                    nodes[parent->parent_entity].children.insert(&nodes[e]);
                }
            }

            hierarchy_dirty = false;
        }
        for(auto& n : root_nodes) {
            imguiTreeNode(getWorld(), n, selected);
        }
    }
};


#endif
