#ifndef ECS_SYSTEM_HPP
#define ECS_SYSTEM_HPP

#include "tuple.hpp"

#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

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


#include "tuple_map_base.hpp"


template<typename T>
class ecsTupleMap : public ecsTupleMapBase {
public:
    typedef std::vector<T*>                         array_t;
    typedef std::unordered_map<entity_id, size_t>   map_t;

protected:
    array_t                                 array;
    map_t                                   map;

public:
    ~ecsTupleMap() {
        for(int i = 0; i < array.size(); ++i) {
            delete array[i];
        }
    }

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
    void signalTupleUpdate(uint32_t array_index, uint64_t attrib_sig) override {
        array[array_index]->signalAttribUpdate(attrib_sig);
    }
    void markDirty(uint32_t array_index) override {
        --dirty_index;
        auto replaced_ptr = array[dirty_index];
        auto moved_ptr    = array[array_index];
        // TODO: To remove later
        map[replaced_ptr->getEntityUid()] = array_index;
        map[moved_ptr->getEntityUid()]    = dirty_index;
        //
        
        array[dirty_index] = moved_ptr;
        moved_ptr->array_index = dirty_index;
        array[array_index] = replaced_ptr;
        replaced_ptr->array_index = array_index;        
    }

    T* create(ecsWorld* world, entity_id ent) {
        //LOG(this << ": create " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T();
        arch_ptr->init(world, ent);
        arch_ptr->array_index = array.size();
        map[ent] = array.size();
        array.resize(array.size() + 1);
        array[array.size() - 1] = arch_ptr;
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
        return array[it->second];
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
            delete array[array.size() - 1];
            array.resize(array.size() - 1);
            map[array[erase_pos]->getEntityUid()] = erase_pos;
        } else {
            delete array[array.size() - 1];
            array.resize(array.size() - 1);
        }
    }
};

template<typename T>
void ecsMarkTupleDirty(
    ecsWorld* world,
    typename ecsTupleMap<T>::map_t& map,
    typename ecsTupleMap<T>::array_t& array_,
    uint32_t& dirty_index,
    entity_id ent,
    uint64_t attrib_sig,
    uint64_t array_idx
) {
    auto& idx = array_[array_idx]->array_index;
    auto& dirty_sig = array_[array_idx]->dirty_signature;
    if(idx < dirty_index) {
        --dirty_index;
        auto last = array_[dirty_index];
        auto moved = array_[array_idx];
        last->array_index = idx;
        moved->array_index = dirty_index;
        moved->dirty_signature |= attrib_sig;
        map[ent] = dirty_index;                        // Update map
        map[last->getEntityUid()] = idx;        // 
        array_[dirty_index] = moved;
        array_[idx] = last;

        // Dirty children recursively
        auto child = world->getFirstChild(ent);
        while(child != NULL_ENTITY) {
            auto it = map.find(child);
            if(it != map.end()) {
                ecsMarkTupleDirty<T>(
                    world,
                    map, 
                    array_,
                    dirty_index, 
                    child, attrib_sig, it->second
                );
            }
            child = world->getNextSibling(child);
        }
    } else if ((dirty_sig & attrib_sig) == 0) {
        dirty_sig |= attrib_sig;
    }
}

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
                world->_unlinkTupleContainer(ent, (ecsTupleMap<Arg>*)this);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && added_requirement) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                ptr->dirty_signature = entity_sig;
                world->_linkTupleContainer(ent, (ecsTupleMap<Arg>*)this, ptr);
                onFit(ptr);
                if(has_optionals) {
                    ptr->updateOptionals(world, ent);
                }
            }
            if(fits && added_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {// Probably never NULL, better be safe
                    ptr->dirty_signature |= (diff_sig & opt_mask); // Set dirty flags for added optionals
                }
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
                // No need to clear dirty flags for optionals, tuple is being removed anyway
                onUnfit(ptr);
                world->_unlinkTupleContainer(ent, (ecsTupleMap<Arg>*)this);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && removed_excluder) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                ptr->dirty_signature = entity_sig;
                world->_linkTupleContainer(ent, (ecsTupleMap<Arg>*)this, ptr);
                onFit(ptr);
            }
            if(fits && removed_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {
                    ptr->clearOptionals(diff_sig);
                    ptr->dirty_signature &= ~(diff_sig & opt_mask); // Clear dirty flags for removed optionals
                }
            }
        }
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static() | Arg::get_optional_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsTupleMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                ecsMarkTupleDirty<Arg>(
                    world,
                    ecsTupleMap<Arg>::map, 
                    ecsTupleMap<Arg>::array,
                    ecsTupleMap<Arg>::dirty_index,
                    ent, attrib_sig, it->second
                );
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
                world->_unlinkTupleContainer(ent, (ecsTupleMap<Arg>*)this);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && added_requirement) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                ptr->dirty_signature = entity_sig;
                world->_linkTupleContainer(ent, (ecsTupleMap<Arg>*)this, ptr);
                onFit(ptr);
                if(has_optionals) {
                    ptr->updateOptionals(world, ent);
                }
            }
            if(fits && added_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {// Probably never NULL, better be safe
                    ptr->dirty_signature |= (diff_sig & opt_mask); // Set dirty flags for added optionals
                }
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
                // No need to clear dirty flags for optionals, tuple is being removed anyway
                onUnfit(ptr);
                world->_unlinkTupleContainer(ent, (ecsTupleMap<Arg>*)this);
                ecsTupleMap<Arg>::erase(ent);
            }
        } else {
            if(fits && removed_excluder) {
                Arg* ptr = ecsTupleMap<Arg>::create(world, ent);
                ptr->dirty_signature = entity_sig;
                world->_linkTupleContainer(ent, (ecsTupleMap<Arg>*)this, ptr);
                onFit(ptr);
            }
            if(fits && removed_optional) {
                Arg* ptr = ecsTupleMap<Arg>::get(ent);
                if(ptr) {
                    ptr->clearOptionals(diff_sig);
                    ptr->dirty_signature &= ~(diff_sig & opt_mask); // Clear dirty flags for removed optionals
                }
            }
        }
        
        ecsSystemRecursive<Args...>::attribsRemoved(world, ent, entity_sig, diff_sig);
    }

    void signalUpdate(entity_id ent, uint64_t attrib_sig) {
        uint64_t arch_sig = Arg::get_signature_static() | Arg::get_optional_signature_static();
        if((arch_sig & attrib_sig) != 0) {
            auto map = ecsTupleMap<Arg>::map;
            auto it = map.find(ent);
            if(it != map.end()) {
                ecsMarkTupleDirty<Arg>(
                    world,
                    ecsTupleMap<Arg>::map, 
                    ecsTupleMap<Arg>::array,
                    ecsTupleMap<Arg>::dirty_index,
                    ent, attrib_sig, it->second
                );
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
    std::vector<ARCH_T*>& get_array() {
        return ecsTupleMap<ARCH_T>::array;
    }
    template<typename ARCH_T>
    int count() const {
        return ecsTupleMap<ARCH_T>::array.size();
    }
    template<typename ARCH_T>
    ARCH_T* get(int i) {
        return ecsTupleMap<ARCH_T>::array[i];
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
        return ecsTupleMap<ARCH_T>::array[it->second];
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
