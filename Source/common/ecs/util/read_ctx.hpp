#ifndef ECS_WORLD_READ_CTX_HPP
#define ECS_WORLD_READ_CTX_HPP

#include "../../util/data_stream.hpp"
#include "../types.hpp"
#include "../attrib_lib.hpp"
#include "../../util/log.hpp"
#include <unordered_map>


class ecsAttribBase;
class ecsWorld;
class ecsWorldReadCtx {
    ecsWorld* world = 0;
    in_stream* strm = 0;
    std::unordered_map<attrib_id, std::string> attrib_names;
    std::unordered_map<entity_id, entity_id> old_to_new_id;


public:
    ecsWorldReadCtx(ecsWorld* world, in_stream* s)
    : world(world), strm(s) {}

    ecsWorld*  getWorld() { return world; }
    in_stream* getStream() { return strm; }

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

    uint64_t convertMask(uint64_t mask) {
        uint64_t converted = 0;
        for (int i = 0; i < (int)attrib_names.size(); ++i) {
            if (mask & (1ULL << i)) {
                auto& name = getAttribName(i);
                attrib_id attrib_index = getEcsAttribTypeLib().get_attrib_id(name.c_str());
                if (attrib_index == -1) {
                    LOG_ERR("Unknown attribute: " << name);
                    continue;
                }
                converted |= (1ULL << attrib_index);
            }
        }
        return converted;
    }

    void setAttribName(attrib_id id, const char* name) {
        attrib_names[id] = name;
    }
    const std::string& getAttribName(attrib_id id) {
        auto it = attrib_names.find(id);
        if(it == attrib_names.end()) {
            return "";
        }
        return it->second;
    }

    void skipBytes(uint64_t count) {
        strm->readArray<char>(count);
    }

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(T& value) {
        strm->read<T>(value);
        return true;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool readArray(std::vector<T>& value) {
        uint64_t count = strm->read<uint64_t>();
        strm->read(value, count);
        return true;
    }
    bool readStr(std::string& value) {
        uint64_t len = strm->read<uint64_t>();
        strm->read(value, len);
        return true;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    T read() {
        T v;
        read(v);
        return v;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    std::vector<T> readArray() {
        std::vector<T> v;
        read(v);
        return v;
    }
    std::string readStr() {
        std::string s;
        readStr(s);
        return s;
    }
    template<typename T>
    std::shared_ptr<T> readResource() {
        ECS_SERIALIZED_RESOURCE_DATA_TYPE storage_type = ECS_RESOURCE_NULL;
        storage_type = (ECS_SERIALIZED_RESOURCE_DATA_TYPE)read<uint8_t>();

        if(storage_type == ECS_RESOURCE_NULL) {
            return std::shared_ptr<T>();
        } else if(storage_type == ECS_RESOURCE_EMBEDDED) {
            std::shared_ptr<T> ptr(new T());
            ptr->deserialize(*strm, strm->bytes_available());
            return ptr;
        } else if(storage_type == ECS_RESOURCE_REFERENCE) {
            std::string r_name = readStr();
            std::shared_ptr<T> ptr = retrieve<T>(r_name);
            return ptr;
        } else {
            assert(NULL);
            return std::shared_ptr<T>();
        }
    }
    
    entity_id readEntityRef() {
        uint64_t e = read<uint64_t>();
        if(e == SERIALIZED_ENTITY_ERROR) {
            return ENTITY_ERROR;
        } else {
            return getRemappedEntityId((entity_id)e);
        }
    }

    entity_id readAttribRef();
};


#endif
