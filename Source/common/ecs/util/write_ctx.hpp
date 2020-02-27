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
public:
    ecsWorldWriteCtx(ecsWorld* world, out_stream* s)
    : world(world), strm(s) {}

    out_stream* getStream() { return strm; }

    void remapEntityId(entity_id oldId, entity_id newId) {
        old_to_new_id[oldId] = newId;
    }
    entity_id getRemappedEntityId(entity_id oldId) {
        auto it = old_to_new_id.find(oldId);
        if(it == old_to_new_id.end()) {
            return (entity_id)-1;
        } else {
            return it->second;
        }
    }

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value) {
        strm->write(value);
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void writeArray(const std::vector<T>& values) {
        strm->write<uint64_t>((uint64_t)values.size());
        strm->write(values);
    }
    void writeStr(const std::string& value) {
        strm->write<uint64_t>((uint64_t)value.size());
        strm->write(value);
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
            resource->serialize(*strm);
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
