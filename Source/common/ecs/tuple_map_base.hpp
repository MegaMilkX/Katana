#ifndef ECS_TUPLE_MAP_BASE_HPP
#define ECS_TUPLE_MAP_BASE_HPP

#include <stdint.h>


class ecsTupleMapBase {
public:
    uint32_t dirty_index = 0;

    virtual ~ecsTupleMapBase() {}
    virtual uint64_t get_mask() const = 0;
    virtual uint64_t get_opt_mask() const = 0;
    virtual uint64_t get_exclusion_mask() const = 0;

    virtual void signalTupleUpdate(uint32_t array_index, uint64_t attrib_sig) = 0;
    virtual void markDirty(uint32_t array_index) = 0;

    virtual void erase(uint32_t array_index) = 0;

    virtual void onUnfitProxy(uint32_t array_index) = 0;
};


#endif
