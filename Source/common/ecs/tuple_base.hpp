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
