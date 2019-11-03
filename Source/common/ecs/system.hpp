#ifndef ECS_SYSTEM_HPP
#define ECS_SYSTEM_HPP

#include "archetype.hpp"

#include <map>

class ecsWorld;

class ecsSystemBase {
    friend ecsWorld;
protected:
    ecsWorld* world = 0;

public:
    virtual ~ecsSystemBase() {}
    virtual void tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) = 0;
    virtual void signalUpdate(entity_id ent, uint64_t attrib_sig) = 0;

    ecsWorld* getWorld() const { return world; }

    virtual void onUpdate() {

    }
};


class ecsArchetypeMapBase {
public:
    virtual ~ecsArchetypeMapBase() {}
    virtual uint64_t get_exclusion_mask() const = 0;
};


template<typename T>
class ecsArchetypeMap : public ecsArchetypeMapBase {
protected:
    std::vector<std::shared_ptr<T>> array;
    std::unordered_map<entity_id, size_t> map;
public:
    uint64_t get_exclusion_mask() const {
        return T::get_exclusion_signature_static();
    }

    T* insert(entity_id ent, const T& arch) {
        //LOG(this << ": insert " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T(arch);
        map[ent] = array.size();
        array.resize(array.size() + 1);
        array[array.size() - 1].reset(arch_ptr);
        return arch_ptr;
    }

    T* get(entity_id ent) {
        auto it = map.find(ent);
        if(it == map.end()) {
            return 0;
        }
        return array[it->second].get();
    }

    void erase(entity_id ent) {
        //LOG(this << ": erase " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        auto it = map.find(ent);
        if(it == map.end()) {
            return;
        }
        size_t erase_pos = it->second;
        map.erase(it);
        if(erase_pos < array.size() - 1) {
            array[erase_pos] = array[array.size() - 1];
            array.resize(array.size() - 1);
            map[array[erase_pos]->getEntityUid()] = erase_pos;
        } else {
            array.resize(array.size() - 1);
        }
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
                fit = true;                
            }
        }

        if(fit) {
            if(ecsArchetypeMap<Arg>::get(ent) == 0) {
                Arg arg;
                arg.init(getWorld(), ent);
                auto ptr = ecsArchetypeMap<Arg>::insert(ent, arg);
                onFit(ptr);
            }
        } else {
            Arg* ptr = ecsArchetypeMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsArchetypeMap<Arg>::erase(ent);
            }
        }
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsArchetypeMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                ecsArchetypeMap<Arg>::array[it->second]->signalAttribUpdate(attrib_sig);
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
                fit = true;
            }
        }
        if(fit) {
            if(ecsArchetypeMap<Arg>::get(ent) == 0) {
                Arg arg;
                arg.init(getWorld(), ent);
                auto ptr = ecsArchetypeMap<Arg>::insert(ent, arg);
                onFit(ptr);
            }
        } else {
            Arg* ptr = ecsArchetypeMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsArchetypeMap<Arg>::erase(ent);
            }
        }

        ecsSystemRecursive<Args...>::tryFit(world, ent, entity_sig);
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsArchetypeMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                ecsArchetypeMap<Arg>::array[it->second]->signalAttribUpdate(attrib_sig);
            }
        }

        ecsSystemRecursive<Args...>::signalUpdate(ent, attrib_sig);
    }
};

template<typename... Args>
class ecsSystem : public ecsSystemRecursive<Args...> {
public:
    template<typename ARCH_T>
    std::vector<std::shared_ptr<ARCH_T>>& get_array() {
        return ecsArchetypeMap<ARCH_T>::array;
    }
};

#endif
