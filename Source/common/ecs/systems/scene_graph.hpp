#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"
#include "../storage/storage_transform.hpp"


class ecsysSceneGraph;
class ecsTupleSubGraph : public ecsTuple<ecsSubScene> {
public:
    ecsysSceneGraph* sub_system = 0;

    void onAttribUpdate(ecsSubScene* ss) {
        if(ss->getWorld()) {
            sub_system = ss->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }
};

class ecsysSceneGraph : public ecsSystem<
    ecsTuple<ecsWorldTransform>,
    ecsTuple<ecsParentTransform>,
    tupleTransform,
    ecsTupleSubGraph
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


    void onFit(ecsTupleSubGraph* sub_graph) {
        if(sub_graph->get<ecsSubScene>()->getWorld()) {
            sub_graph->sub_system = sub_graph->get<ecsSubScene>()->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }

    void onFit(tupleTransform* o) {
        o->system = this;
    }

    void onFit(ecsTuple<ecsParentTransform>* o) {
        hierarchy_dirty = true;
    }
    void onUnfit(ecsTuple<ecsParentTransform>* o) {
        hierarchy_dirty = true;
    }

    void onFit(ecsTuple<ecsWorldTransform>* o) {
    }
    void onUnfit(ecsTuple<ecsWorldTransform>* o) {        
        for(auto& a : get_array<ecsTuple<ecsParentTransform>>()) {
            if(a->get<ecsParentTransform>()->parent_entity == o->getEntityUid()) {
                world->removeAttrib(a->getEntityUid(), ecsParentTransform::get_id_static());
            }
        }
        hierarchy_dirty = true;
    }


    void onUpdate() {
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

        for(auto& a : get_array<ecsTupleSubGraph>()) {
            if(!a->sub_system) continue;
            a->sub_system->onUpdate();
        }

        for(int i = count<tupleTransform>() - 1; i >= get_dirty_index<tupleTransform>(); --i) {
            auto tuple = get<tupleTransform>(i);
            ecsTranslation* translation     = tuple->get_optional<ecsTranslation>();
            ecsRotation* rotation           = tuple->get_optional<ecsRotation>();
            ecsScale* scale                 = tuple->get_optional<ecsScale>();
            ecsWorldTransform* world        = tuple->get<ecsWorldTransform>();

            gfxm::mat4 local = gfxm::mat4(1.0f);
            if(translation) {
                local = gfxm::translate(local, translation->getPosition());
            }
            if(rotation) {
                local = local * gfxm::to_mat4(rotation->getRotation());
            }
            if(scale) {
                local = gfxm::scale(local, scale->getScale());
            }
            world->transform = local;

            if(tuple->get_parent()) {
                world->transform = tuple->get_parent()->get<ecsWorldTransform>()->transform * world->transform;
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<tupleTransform>();
    }

    void imguiTreeNode(ecsWorld* world, Node* node, entity_id* selected) {
        ecsName* name_attrib = getWorld()->findAttrib<ecsName>(node->id);
        std::string entity_name = "[ANONYMOUS]";
        if(name_attrib) {
            if(name_attrib->name.size()) {
                entity_name = name_attrib->name;
            } else {
                entity_name = "[EMPTY_NAME]";
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
        entity_id ent = world->createEntity().getId();
        hierarchy_dirty = true;
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
