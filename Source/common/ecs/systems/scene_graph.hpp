#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"


class ecsysSceneGraph;
class tupleTransform : public ecsTuple<
    ecsOptional<ecsParentTransform>, 
    ecsOptional<ecsTranslation>,
    ecsOptional<ecsRotation>,
    ecsOptional<ecsScale>, 
    ecsWorldTransform
> {
    friend ecsysSceneGraph;
    ecsysSceneGraph* system = 0;
    tupleTransform* parent = 0;
    std::set<tupleTransform*> children;
    size_t dirty_index;
public:
    ~tupleTransform() {
        if(parent) {
            parent->children.erase(this);
        }
        for(auto c : children) {
            c->parent = 0;
        }
    }
    void onAddOptional(ecsParentTransform* p) override;
    void onRemoveOptional(ecsParentTransform* p) override;

    void onAddOptional(ecsTranslation* trs) override {
        trs->system = system;
        trs->dirty_index = dirty_index;
    }
    void onAddOptional(ecsRotation* trs) override {
        trs->system = system;
        trs->dirty_index = dirty_index;
    }
    void onAddOptional(ecsScale* trs) override {
        trs->system = system;
        trs->dirty_index = dirty_index;
    }


    void setDirtyIndex(size_t i) {
        dirty_index = i;
        if(get_optional<ecsTranslation>()) {
            get_optional<ecsTranslation>()->dirty_index = i;
        }
        if(get_optional<ecsRotation>()) {
            get_optional<ecsRotation>()->dirty_index = i;
        }
        if(get_optional<ecsScale>()) {
            get_optional<ecsScale>()->dirty_index = i;
        }
    }
};

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

    std::vector<tupleTransform*> dirty_vec;
    size_t first_dirty_index = 0;

    void onFit(ecsTupleSubGraph* sub_graph) {
        if(sub_graph->get<ecsSubScene>()->getWorld()) {
            sub_graph->sub_system = sub_graph->get<ecsSubScene>()->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }

    void onFit(tupleTransform* o) {
        o->system = this;
        o->dirty_index = dirty_vec.size();
        dirty_vec.emplace_back(o); 
    }
    void onUnfit(tupleTransform* o) {/*
        uint64_t current_index = o->dirty_index;
        dirty_vec.back()->setDirtyIndex(current_index);
        dirty_vec[current_index] = dirty_vec.back();
        dirty_vec.resize(dirty_vec.size() - 1);*/
        if (o->dirty_index < first_dirty_index) {
            --first_dirty_index;
        }
        dirty_vec.erase(dirty_vec.begin() + o->dirty_index);
        for (int i = o->dirty_index; i < dirty_vec.size(); ++i) {
            dirty_vec[i]->setDirtyIndex(dirty_vec[i]->dirty_index - 1);
        }
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

        for(size_t i = first_dirty_index; i < dirty_vec.size(); ++i) {
            auto a = dirty_vec[i];

            ecsTranslation* translation = a->get_optional<ecsTranslation>();
            ecsRotation* rotation = a->get_optional<ecsRotation>();
            ecsScale* scale = a->get_optional<ecsScale>();
            ecsWorldTransform* world = a->get<ecsWorldTransform>();

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
        }

        for(size_t i = first_dirty_index; i < dirty_vec.size(); ++i) {
            auto a = dirty_vec[i];

            ecsParentTransform* parent_transform = a->get_optional<ecsParentTransform>();
            if(!parent_transform) continue;
            ecsWorldTransform* parent_world = parent_transform->parent_transform;
            ecsWorldTransform* world = a->get<ecsWorldTransform>();

            if(parent_world) {
                world->transform = parent_world->transform * world->transform;
            }
        }

        first_dirty_index = dirty_vec.size();
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

    void setDirtyIndexRecursive(tupleTransform* o) {
        for(auto c : o->children) {
            setDirtyIndexRecursive(c);
        }

        //first_dirty_index = o->;
        uint64_t current_index = o->dirty_index;
        uint64_t new_index = std::max((size_t)0, std::min(first_dirty_index - 1, dirty_vec.size() - 1));
        if(current_index >= first_dirty_index) {
            return;
        }

        tupleTransform* tmp = dirty_vec[new_index];
        dirty_vec[new_index] = dirty_vec[current_index];
        dirty_vec[current_index] = tmp;
        tmp->setDirtyIndex(current_index);
        o->setDirtyIndex(new_index);

        first_dirty_index = new_index;
    }

public:
    void setDirtyIndex(size_t index) {
        if(index < first_dirty_index) {
            setDirtyIndexRecursive(dirty_vec[index]);
        }
    }

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


inline void tupleTransform::onAddOptional(ecsParentTransform* p) {
    parent = system->get_tuple<tupleTransform>(p->parent_entity);
    if (parent) {
        parent->children.insert(this);
    }
}
inline void tupleTransform::onRemoveOptional(ecsParentTransform* p) {
    if (parent) {
        parent->children.erase(this);
    }
    parent = 0;
    system->setDirtyIndex(dirty_index);
}


#endif
