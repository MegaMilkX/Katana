#ifndef ECS_WORLD_WRITE_CTX_HPP
#define ECS_WORLD_WRITE_CTX_HPP

#include "../../util/data_stream.hpp"
#include "../types.hpp"

#include <unordered_map>


class ecsAttribBase;
class ecsWorld;
class ecsWorldWriteCtx {
    ecsWorld* world = 0;
    out_stream* strm = 0;
    std::unordered_map<entity_id, entity_id> old_to_new_id;
    dstream subblock_stream;
    bool is_in_subblock = false;
    out_stream* current_stream = 0;
public:
    ecsWorldWriteCtx(ecsWorld* world, out_stream* s)
    : world(world), strm(s), current_stream(s) {}

    ecsWorld*   getWorld() { return world; }
    out_stream* getStream() { return current_stream; }

    void remapEntityId(entity_id oldId, entity_id newId) {
        old_to_new_id[oldId] = newId;
    }
    entity_id getRemappedEntityId(entity_id oldId) {
        auto it = old_to_new_id.find(oldId);
        if(it == old_to_new_id.end()) {
            return NULL_ENTITY;
        } else {
            return it->second;
        }
    }

    void beginSubBlock();
    void endSubBlock();

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value) {
        current_stream->write(value);
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void writeArray(const std::vector<T>& values) {
        current_stream->write<uint64_t>((uint64_t)values.size());
        current_stream->write(values);
    }
    void writeStr(const std::string& value) {
        current_stream->write<uint64_t>((uint64_t)value.size());
        current_stream->write(value);
    }
    template<typename T>
    void writeResource(const std::shared_ptr<T>& resource) {
        std::string res_name;
        if(!resource) {
            write<uint8_t>(ECS_RESOURCE_NULL);
            return;
        }
        res_name = resource->Name();
        if(res_name.empty()) {
            write<uint8_t>(ECS_RESOURCE_EMBEDDED);
            resource->serialize(*current_stream);
        } else {
            write<uint8_t>(ECS_RESOURCE_REFERENCE);
            writeStr(res_name);
        }
    }

    void writeEntityRef(entity_id e) {
        entity_id ent_serialized = getRemappedEntityId(e);
        if(ent_serialized == ENTITY_ERROR) {
            write<uint64_t>(SERIALIZED_ENTITY_ERROR);
        } else {
            write<uint64_t>(ent_serialized);
        }
    }

    void writeAttribRef(ecsAttribBase* attrib);

};


#endif
