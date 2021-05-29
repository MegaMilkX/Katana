#ifndef ECS_BEHAVIOR_SYSTEM_HPP
#define ECS_BEHAVIOR_SYSTEM_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

class ecsTplBhvr : public ecsTuple<ecsBehavior> {};

class ecsBehaviorSys : public ecsSystem<ecsTplBhvr>{
public:
    void onUpdate(float dt) override {
        for(int i = get_dirty_index<ecsTplBhvr>(); i < count<ecsTplBhvr>(); ++i) {
            auto b = get<ecsTplBhvr>(i);
            auto bhvr = b->get<ecsBehavior>();
            if(bhvr->bhvr) {
                bhvr->bhvr->onBegin();
            }
        }
        clear_dirty<ecsTplBhvr>();
    }

};


#endif
