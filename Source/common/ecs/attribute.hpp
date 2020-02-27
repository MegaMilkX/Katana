#ifndef ECS_ATTRIBUTE_HPP
#define ECS_ATTRIBUTE_HPP

#include "../lib/imgui_wrap.hpp"
#include "attrib_lib.hpp"
#include "../gui_viewport.hpp"

#include "../util/data_stream.hpp"

#include <stdint.h>

#include "types.hpp"
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

class ecsAttribBase {
    friend ecsEntity;

    entity_id entity = (entity_id)-1;

public:
    virtual ~ecsAttribBase() {}
    virtual uint64_t get_id() const = 0;

    entity_id getEntityId() { return entity; }

    virtual void onGui(ecsWorld* world, entity_id ent) {}

    virtual void write(ecsWorldWriteCtx& out) {}
    virtual void read(ecsWorldReadCtx& in) {}
};

template<typename T>
class ecsAttrib : public ecsAttribBase {
public:
    static uint64_t get_id_static() {
        static uint64_t id = next_attrib_id();
        return id;
    } 
    uint64_t get_id() const override {
        return get_id_static();
    }
};


#endif
