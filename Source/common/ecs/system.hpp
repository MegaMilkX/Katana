#ifndef ECS_SYSTEM_HPP
#define ECS_SYSTEM_HPP

#include "archetype.hpp"

#include <map>

class ecsWorld;

class ecsSystemBase {
public:
    virtual ~ecsSystemBase() {}
    virtual void tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) = 0;

    virtual void onUpdate() {

    }
};


template<typename T>
class ecsArchetypeMap {
protected:
    std::map<entity_id, std::shared_ptr<T>> values;
public:
    T* insert(entity_id ent, const T& arch) {
        LOG(this << ": insert " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T(arch);
        values[ent].reset(arch_ptr);
        return arch_ptr;
    }

    T* get(entity_id ent) {
        auto it = values.find(ent);
        if(it == values.end()) {
            return 0;
        }
        return it->second.get();
    }

    void erase(entity_id ent) {
        LOG(this << ": erase " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        values.erase(ent);
    }
};

template<typename... Args>
class ecsSystemRecursive;

template<typename Arg>
class ecsSystemRecursive<Arg> 
: public ecsArchetypeMap<Arg>, public ecsSystemBase {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    void tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        uint64_t exclusion_arch_sig = Arg::get_exclusion_signature_static();
        bool fit = false;
        if((arch_sig & entity_sig) == arch_sig) {
            if((exclusion_arch_sig & entity_sig) == 0) {
                if(ecsArchetypeMap<Arg>::get(ent) == 0) {
                    auto ptr = ecsArchetypeMap<Arg>::insert(ent, Arg(world->getEntity(ent)));
                    onFit(ptr);
                }
                fit = true;
            }
        }
        if(!fit) {
            Arg* ptr = ecsArchetypeMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsArchetypeMap<Arg>::erase(ent);
            }
        }
    }
};

template<typename Arg, typename... Args>
class ecsSystemRecursive<Arg, Args...> 
: public ecsArchetypeMap<Arg>, public ecsSystemRecursive<Args...> {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    void tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        uint64_t exclusion_arch_sig = Arg::get_exclusion_signature_static();
        bool fit = false;
        if((arch_sig & entity_sig) == arch_sig) {
            if((exclusion_arch_sig & entity_sig) == 0) {
                if(ecsArchetypeMap<Arg>::get(ent) == 0) {
                    auto ptr = ecsArchetypeMap<Arg>::insert(ent, Arg(world->getEntity(ent)));
                    onFit(ptr);
                }
                fit = true;
            }
        }
        if(!fit) {
            Arg* ptr = ecsArchetypeMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsArchetypeMap<Arg>::erase(ent);
            }
        }

        ecsSystemRecursive<Args...>::tryFit(world, ent, entity_sig);
    }
};

template<typename... Args>
class ecsSystem : public ecsSystemRecursive<Args...> {
public:
    template<typename ARCH_T>
    std::map<entity_id, std::shared_ptr<ARCH_T>>& get_archetype_map() {
        return ecsArchetypeMap<ARCH_T>::values;
    }
};

#endif
