#ifndef ECS_TUPLE_TRANSFORM_HPP
#define ECS_TUPLE_TRANSFORM_HPP


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


#endif
