#ifndef ECS_TUPLE_BASE_HPP
#define ECS_TUPLE_BASE_HPP

#include "types.hpp"

class ecsWorld;

class ecsTupleBase {
protected:
    entity_id entity_uid;
public:
    uint32_t        array_index         = 0;
    uint64_t        dirty_signature     = 0;
    ecsTupleBase*   parent              = 0;
    ecsTupleBase*   prev_sibling        = 0;
    ecsTupleBase*   next_sibling        = 0;
    ecsTupleBase*   first_child         = 0;

    void _addChild(ecsTupleBase* e) {
        e->parent = this;
        if(!first_child) {
            first_child = e;
            return;
        }
        auto child = first_child;
        while(child != 0) { 
            if(!child->next_sibling) {
                child->next_sibling = e;
                e->prev_sibling = child;
                break;
            }
            child = child->next_sibling;
        }
    }
    void _removeChild(ecsTupleBase* e) {
        if (first_child == e) {
            auto next = first_child->next_sibling;
            first_child = next;
            if (first_child) {
                first_child->prev_sibling = 0;
            }
            return;
        }

        auto child = first_child;
        while (child != 0) {
            auto next = child->next_sibling;
            if (next == e) {
                child->next_sibling = next->next_sibling;
                next->prev_sibling = child;
                return;
            }
            child = child->next_sibling;
        }
    }

    virtual ~ecsTupleBase() {}
    virtual uint64_t get_signature() const = 0;
    virtual uint64_t get_optional_signature() const = 0;
    virtual uint64_t get_exclusion_signature() const = 0;

    template<typename T>
    bool is_dirty() const {
        return (dirty_signature & (1 << T::get_id_static())) != 0;
    }
    void clear_dirty_signature() {
        dirty_signature = 0;
    }

    virtual void init(ecsWorld* world, entity_id ent) = 0;
    virtual void updateOptionals(ecsWorld* world, entity_id ent) = 0;
    virtual void clearOptionals(uint64_t mask) = 0;

    entity_id getEntityUid() const { return entity_uid; }

    virtual void signalAttribUpdate(uint64_t attrib_sig) = 0;

};


#endif
