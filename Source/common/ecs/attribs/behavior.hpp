#ifndef ECS_BEHAVIOR_HPP
#define ECS_BEHAVIOR_HPP


#include "../attribute.hpp"
#include "../../util/imgui_helpers.hpp"

#include "../../lib/rttr_wrap.hpp"

class ecsBehavior;
class BehaviorBase {
friend ecsBehavior; 
    RTTR_ENABLE()
    ecsEntityHandle entity_hdl;

public:
    virtual ~BehaviorBase() {}
    ecsEntityHandle& getEntityHdl() { return entity_hdl; }

    virtual void onBegin() {}
    virtual void onEnd() {}

};/*
STATIC_RUN(BehaviorBase) {
    rttr::registration::class_<BehaviorBase>("BehaviorBase")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}*/


class ecsBehaviorSys;
class ecsBehavior : public ecsAttrib<ecsBehavior> {
friend ecsBehaviorSys;
    std::unique_ptr<BehaviorBase> bhvr;

public:
    void setBehavior(BehaviorBase* ptr) {
        if(bhvr) {
            bhvr->onEnd();
        }
        bhvr.reset(ptr);
        ptr->entity_hdl = getEntityHdl();
        getEntityHdl().signalUpdate<ecsBehavior>();
    }
    BehaviorBase* getBehavior() {
        return bhvr.get();
    }

    void write(ecsWorldWriteCtx& out) override {
        if(bhvr) {
            out.writeStr(bhvr->get_type().get_name().to_string());
        } else {
            out.writeStr("");
        }
    }
    void read(ecsWorldReadCtx& in) override {
        std::string tname = in.readStr();
        rttr::type t = rttr::type::get_by_name(tname.c_str());
        if(!t.is_valid()) {
            return;
        }
        auto variant = t.create();
        if(!variant.is_valid()) {
            return;
        }
        setBehavior(variant.get_value<BehaviorBase*>());
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        auto derived = rttr::type::get<BehaviorBase>().get_derived_classes();
        std::string preview_name = "<null>";
        if(bhvr) {
            preview_name = bhvr->get_type().get_name().to_string();
        }
        if(ImGui::BeginCombo("behavior", preview_name.c_str())) {
            for(auto& d : derived) {
                if(ImGui::Selectable(d.get_name().to_string().c_str())) {
                    setBehavior(d.create().get_value<BehaviorBase*>());
                }
            }
            ImGui::EndCombo();
        }
    }

};


#endif
