#ifndef ECS_SYSTEM_HPP
#define ECS_SYSTEM_HPP

#include "tuple.hpp"

#include <map>

class ecsWorld;

class ecsSystemBase {
    friend ecsWorld;
protected:
    ecsWorld* world = 0;

public:
    virtual ~ecsSystemBase() {}

    virtual uint64_t get_mask() const = 0;
    virtual uint64_t get_opt_mask() const = 0;
    virtual uint64_t get_exclusion_mask() const = 0;

    virtual void attribsCreated(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) = 0;
    virtual void attribsRemoved(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) = 0;

    virtual void signalUpdate(entity_id ent, uint64_t attrib_sig) = 0;

    ecsWorld* getWorld() const { return world; }

    virtual void onUpdate() {

    }
};


class ecsTupleMapBase {
public:
    virtual ~ecsTupleMapBase() {}
    virtual uint64_t get_mask() const = 0;
    virtual uint64_t get_opt_mask() const = 0;
    virtual uint64_t get_exclusion_mask() const = 0;
};


template<typename T>
class ecsTupleMap : public ecsTupleMapBase {
protected:
    uint32_t                                dirty_index = 0;
    std::vector<std::shared_ptr<T>>         array;
    std::unordered_map<entity_id, size_t>   map;

    std::vector<ecsTupleMap<T>>             sub_world_tuples_array;
    std::unordered_map<entity_id, size_t>   sub_world_tuples_map;

public:
    void clear_dirty_index() {
        dirty_index = array.size();
    }

    uint64_t get_mask() const override {
        return T::get_signature_static();
    }
    uint64_t get_opt_mask() const override {
        return T::get_optional_signature_static();
    }
    uint64_t get_exclusion_mask() const override {
        return T::get_exclusion_signature_static();
    }

    T* create(ecsWorld* world, entity_id ent) {
        //LOG(this << ": create " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T();
        arch_ptr->init(world, ent);
        arch_ptr->array_index = array.size();
        map[ent] = array.size();
        array.resize(array.size() + 1);
        array[array.size() - 1].reset(arch_ptr);
        return arch_ptr;
    }

    T* insert(entity_id ent, const T& arch) {
        //LOG(this << ": insert " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T(arch);
        arch_ptr->array_index = array.size();
        map[ent] = array.size();
        array.resize(array.size() + 1);
        array[array.size() - 1].reset(arch_ptr);
        return arch_ptr;
    }

    void updateOptionals(ecsWorld* world, entity_id ent, uint64_t entity_sig) {
        T* value = get(ent);
        if(!value) return;
        value->updateOptionals(world, ent);
        //LOG(this << ": optional updated " << ent << ": " << rttr::type::get<T>().get_name().to_string());
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
: public ecsTupleMap<Arg>, public ecsSystemBase {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    void attribsCreated(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        uint64_t mask = Arg::get_signature_static();
        uint64_t opt_mask = Arg::get_optional_signature_static();
        uint64_t exclusion_mask = Arg::get_exclusion_signature_static();

        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool has_excluders = (entity_sig & exclusion_mask) != 0;
        const bool has_optionals = (entity_sig & opt_mask) != 0;
        const bool fits = requirements_satisfied && !has_excluders;
        const bool added_requirement = (mask & diff_sig) != 0;
        const bool added_optional = (opt_mask & diff_sig) != 0;
        const bool added_excluder = (exclusion_mask & diff_sig) != 0;
        
        if(added_excluder) {
            Arg* ptr = ecsTupleMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && added_requirement) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                onFit(ptr);
                if(has_optionals) {
                    ptr->updateOptionals(world, ent);
                }
            }
            if(fits && added_optional) {
                ecsTupleMap<Arg>::updateOptionals(world, ent, entity_sig);
            }
        }
    }
    void attribsRemoved(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        entity_sig &= ~(diff_sig); // entity_sig is still unchanged at this point, updating it locally

        uint64_t mask = Arg::get_signature_static();
        uint64_t opt_mask = Arg::get_optional_signature_static();
        uint64_t exclusion_mask = Arg::get_exclusion_signature_static();

        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool has_excluders = (entity_sig & exclusion_mask) != 0;
        const bool fits = requirements_satisfied && !has_excluders;
        const bool removed_requirement = (mask & diff_sig) != 0;
        const bool removed_optional = (opt_mask & diff_sig) != 0;
        const bool removed_excluder = (exclusion_mask & diff_sig) != 0;

        if(removed_requirement) {
            Arg* ptr = ecsTupleMap<Arg>::get(ent);
            if(ptr) {
                ptr->clearOptionals((uint64_t)-1);
                onUnfit(ptr);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && removed_excluder) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                onFit(ptr);
            }
            if(fits && removed_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {
                    ptr->clearOptionals(diff_sig);
                }
            }
        }
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsTupleMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                auto& idx = ecsTupleMap<Arg>::array[it->second]->array_index;
                auto& didx = ecsTupleMap<Arg>::dirty_index;
                if(idx < didx) {
                    --didx;
                    auto last = ecsTupleMap<Arg>::array[didx];
                    auto moved = ecsTupleMap<Arg>::array[it->second];
                    last->array_index = idx;
                    moved->array_index = didx;
                    ecsTupleMap<Arg>::array[didx] = moved;
                    ecsTupleMap<Arg>::array[idx] = last;
                }
                ecsTupleMap<Arg>::array[it->second]->signalAttribUpdate(attrib_sig);
            }
        }
    }
};

template<typename Arg, typename... Args>
class ecsSystemRecursive<Arg, Args...> 
: public ecsTupleMap<Arg>, public ecsSystemRecursive<Args...> {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    void attribsCreated(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        uint64_t mask = Arg::get_signature_static();
        uint64_t opt_mask = Arg::get_optional_signature_static();
        uint64_t exclusion_mask = Arg::get_exclusion_signature_static();

        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool has_excluders = (entity_sig & exclusion_mask) != 0;
        const bool has_optionals = (entity_sig & opt_mask) != 0;
        const bool fits = requirements_satisfied && !has_excluders;
        const bool added_requirement = (mask & diff_sig) != 0;
        const bool added_optional = (opt_mask & diff_sig) != 0;
        const bool added_excluder = (exclusion_mask & diff_sig) != 0;
        
        if(added_excluder) {
            Arg* ptr = ecsTupleMap<Arg>::get(ent);
            if(ptr) {
                onUnfit(ptr);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && added_requirement) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                onFit(ptr);
                if(has_optionals) {
                    ptr->updateOptionals(world, ent);
                }
            }
            if(fits && added_optional) {
                ecsTupleMap<Arg>::updateOptionals(world, ent, entity_sig);
            }
        }

        ecsSystemRecursive<Args...>::attribsCreated(world, ent, entity_sig, diff_sig);
    }
    void attribsRemoved(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        entity_sig &= ~(diff_sig); // entity_sig is still unchanged at this point, updating it locally

        uint64_t mask = Arg::get_signature_static();
        uint64_t opt_mask = Arg::get_optional_signature_static();
        uint64_t exclusion_mask = Arg::get_exclusion_signature_static();

        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool has_excluders = (entity_sig & exclusion_mask) != 0;
        const bool fits = requirements_satisfied && !has_excluders;
        const bool removed_requirement = (mask & diff_sig) != 0;
        const bool removed_optional = (opt_mask & diff_sig) != 0;
        const bool removed_excluder = (exclusion_mask & diff_sig) != 0;

        if(removed_requirement) {
            Arg* ptr = ecsTupleMap<Arg>::get(ent);
            if(ptr) {
                ptr->clearOptionals((uint64_t)-1);
                onUnfit(ptr);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && removed_excluder) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                onFit(ptr);
            }
            if(fits && removed_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {
                    ptr->clearOptionals(diff_sig);
                }
            }
        }
        
        ecsSystemRecursive<Args...>::attribsRemoved(world, ent, entity_sig, diff_sig);
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsTupleMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                auto& idx = ecsTupleMap<Arg>::array[it->second]->array_index;
                auto& didx = ecsTupleMap<Arg>::dirty_index;
                if(idx < didx) {
                    --didx;
                    auto last = ecsTupleMap<Arg>::array[didx];
                    auto moved = ecsTupleMap<Arg>::array[it->second];
                    last->array_index = idx;
                    moved->array_index = didx;
                    ecsTupleMap<Arg>::array[didx] = moved;
                    ecsTupleMap<Arg>::array[idx] = last;
                }
                ecsTupleMap<Arg>::array[it->second]->signalAttribUpdate(attrib_sig);
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
        return ecsTupleMap<ARCH_T>::array;
    }
    template<typename ARCH_T>
    int count() const {
        return ecsTupleMap<ARCH_T>::array.size();
    }
    template<typename ARCH_T>
    ARCH_T* get(int i) {
        return ecsTupleMap<ARCH_T>::array[i].get();
    }
    template<typename ARCH_T>
    int get_dirty_index() const {
        return ecsTupleMap<ARCH_T>::dirty_index;
    }
    template<typename ARCH_T>
    void clear_dirty() {
        return ecsTupleMap<ARCH_T>::clear_dirty_index();
    }

    template<typename ARCH_T>
    ARCH_T* get_tuple(entity_id ent) {
        auto it = ecsTupleMap<ARCH_T>::map.find(ent);
        if(it == ecsTupleMap<ARCH_T>::map.end()) {
            return 0;
        }
        return ecsTupleMap<ARCH_T>::array[it->second].get();
    }

    uint64_t get_mask() const override {
        static uint64_t x[] = { ecsTupleMap<Args>::get_mask()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_opt_mask() const override {
        static uint64_t x[] = { ecsTupleMap<Args>::get_opt_mask()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_exclusion_mask() const override {
        static uint64_t x[] = { ecsTupleMap<Args>::get_exclusion_mask()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }

    void attribsCreated(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        if(((get_mask() & diff_sig) == 0) 
            && ((get_opt_mask() & diff_sig) == 0)
            && ((get_exclusion_mask() & diff_sig) == 0)
        ) {
            return;
        }
        ecsSystemRecursive<Args...>::attribsCreated(world, ent, entity_sig, diff_sig);
    }
    void attribsRemoved(ecsWorld* world, entity_id ent, uint64_t entity_sig, uint64_t diff_sig) override {
        if(((get_mask() & diff_sig) == 0) 
            && ((get_opt_mask() & diff_sig) == 0)
            && ((get_exclusion_mask() & diff_sig) == 0)
        ) {
            return;
        }
        ecsSystemRecursive<Args...>::attribsRemoved(world, ent, entity_sig, diff_sig);
    }
};

#endif
