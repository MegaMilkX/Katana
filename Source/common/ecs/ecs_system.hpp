#ifndef KT_ECS_SYSTEM_HPP
#define KT_ECS_SYSTEM_HPP

#include "entity.hpp"
#include "archetype.hpp"

#include <map>
#include <set>

class ktEcsWorld;

class ktEcsISystem {
public:
    virtual ~ktEcsISystem() {}
    virtual void onAttribCreated(ktEcsWorld& world, ktEntity ent, rttr::type t) = 0;
    virtual void onAttribRemoved(ktEcsWorld& world, ktEntity ent, rttr::type t) = 0;
    virtual void onInvoke() {}
};

template<typename... Args>
class ktEcsSystem;

template<typename Arg>
class ktEcsSystem<Arg> : public ktEcsISystem {
protected:
    typedef rttr::type arch_type_id;
    std::map<
        arch_type_id, 
        std::map<
            ktEntity,
            void*
        >
    > arch_objects;
public:
    virtual void onFit(Arg* arch) = 0;
    virtual void onUnfit(Arg* arch) = 0;

    void onAttribCreated(ktEcsWorld& world, ktEntity ent, rttr::type t) override {
        if(Arg::requiresAttrib(t)) {
            if(Arg::entityFits(world, ent)) {
                Arg* arch_obj = new Arg();
                arch_objects[rttr::type::get<Arg>()][ent] = arch_obj;
                onFit(arch_obj);
            }
        }
    }
    void onAttribRemoved(ktEcsWorld& world, ktEntity ent, rttr::type t) override {
        if(Arg::requiresAttrib(t)) {
            auto& archs = arch_objects[rttr::type::get<Arg>()];
            auto it = archs.find(ent);
            if(it != archs.end()) {
                Arg* ptr = (Arg*)it->second;
                onUnfit(ptr);
                delete ptr;
            }
        }
    }
};

template<typename Arg, typename... Args>
class ktEcsSystem<Arg, Args...> : public ktEcsSystem<Args...> {
public:
    virtual void onFit(Arg* arch) = 0;
    virtual void onUnfit(Arg* arch) = 0;

    void onAttribCreated(ktEcsWorld& world, ktEntity ent, rttr::type t) override {
        if(Arg::requiresAttrib(t)) {
            if(Arg::entityFits(world, ent)) {
                Arg* arch_obj = new Arg();
                arch_objects[rttr::type::get<Arg>()][ent] = arch_obj;
                onFit(arch_obj);
            }
        }
        ktEcsSystem<Args...>::onAttribCreated(world, ent, t);
    }
    void onAttribRemoved(ktEcsWorld& world, ktEntity ent, rttr::type t) override {
        if(Arg::requiresAttrib(t)) {
            auto& archs = arch_objects[rttr::type::get<Arg>()];
            auto it = archs.find(ent);
            if(it != archs.end()) {
                Arg* ptr = (Arg*)it->second;
                onUnfit(ptr);
                delete ptr;
            }
        }
        ktEcsSystem<Args...>::onAttribRemoved(world, ent, t);
    }
};

#endif
