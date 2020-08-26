#ifndef ECS_ATTRIBUTE_HPP
#define ECS_ATTRIBUTE_HPP

#include "../lib/imgui_wrap.hpp"
#include "attrib_lib.hpp"
#include "../gui_viewport.hpp"

#include "../util/data_stream.hpp"

#include <stdint.h>

#include "types.hpp"
#include "entity_handle.hpp"
#include "util/write_ctx.hpp"
#include "util/read_ctx.hpp"

inline uint64_t& get_last_attrib_id() {
    static uint64_t id = 0;
    return id;
}

inline void set_last_attrib_id(uint64_t last_id) {
    get_last_attrib_id() = last_id;
}

inline uint64_t next_attrib_id() {
    static uint64_t id = 0;
    set_last_attrib_id(id++);
    return get_last_attrib_id();
}

class ecsWorld;
class ecsEntity;
struct attrib_type_info;

class ecsAttribBase {
    friend ecsEntity;
    friend void constructEntityData(void* data, uint64_t sig, ecsWorld* world, entity_id e);
    friend void copyConstructEntityData(void* dest, uint64_t dest_sig, void* src, uint64_t src_sig, ecsWorld* world, entity_id e);

    ecsEntityHandle h_entity;

public:
    virtual ~ecsAttribBase() {}
    virtual uint64_t get_id() const = 0;
    virtual ecsAttribType get_attrib_type() const = 0;

    entity_id getEntityId() { return h_entity.getId(); }
    ecsEntityHandle getEntityHdl() { return h_entity; }

    virtual void onGui(ecsWorld* world, entity_id ent) {}

    virtual void write(ecsWorldWriteCtx& out) {}
    virtual void read(ecsWorldReadCtx& in) {}
};

template<typename T, ecsAttribType ATTR_TYPE = ecsAttribType::Normal>
class ecsAttrib : public ecsAttribBase {
public:
    static const attrib_type_info* attrib_info;

    static uint64_t get_id_static() {
        static uint64_t id = next_attrib_id();
        return id;
    } 
    uint64_t get_id() const override {
        return get_id_static();
    }
    static ecsAttribType get_attrib_type_static() {
        return ATTR_TYPE;
    }
    ecsAttribType get_attrib_type() const override {
        return get_attrib_type_static();
    }
};
template<typename T, ecsAttribType ATTR_TYPE>
const attrib_type_info* ecsAttrib<T, ATTR_TYPE>::attrib_info;


#endif
