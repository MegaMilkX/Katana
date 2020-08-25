#ifndef ECS_ATTRIB_LIB_HPP
#define ECS_ATTRIB_LIB_HPP

#include <unordered_map>

#include "types.hpp"

class ecsAttribBase;

struct attrib_type_info {
    typedef ecsAttribBase*(*constructor_fn_t)(void);
    typedef void          (*constructor_in_place_fn_t)(ecsAttribBase*);
    typedef void          (*copy_constructor_fn_t)(ecsAttribBase* dest, ecsAttribBase* src);
    
    constructor_fn_t constructor;
    constructor_in_place_fn_t constructor_in_place;
    copy_constructor_fn_t     copy_constructor;
    std::string name;
    size_t size_of = 0;
    ecsAttribType attrib_type;
};

class EcsAttribTypeLib {
    std::unordered_map<std::string, std::vector<attrib_id>> table;
    std::unordered_map<attrib_id, attrib_type_info> attrib_infos;
    std::unordered_map<std::string, attrib_id>      attrib_indices;
public:
    template<typename T>
    void add(const char* category, const char* name) {
        attrib_type_info inf;
        inf.constructor = []()->ecsAttribBase*{
            return new T();
        };
        inf.constructor_in_place = [](ecsAttribBase* ptr){
            new ((T*)ptr) T();
        };
        inf.copy_constructor = [](ecsAttribBase* dest, ecsAttribBase* src){
            new((T*)dest) T(*(T*)src);
        };
        inf.name = name;
        inf.size_of = sizeof(T);
        inf.attrib_type = T::get_attrib_type_static();
        table[category].emplace_back(T::get_id_static());
        attrib_infos[T::get_id_static()] = inf;
        attrib_indices[name] = T::get_id_static();
    }
    const attrib_type_info* get_info(attrib_id id) const {
        const auto it = attrib_infos.find(id);
        if(it == attrib_infos.end()) {
            return 0;
        }
        return &it->second;
    }
    const attrib_type_info* get_info(const char* name) const {
        const auto it = attrib_indices.find(name);
        if(it == attrib_indices.end()) {
            return 0;
        }
        const auto it2 = attrib_infos.find(it->second);
        if(it2 == attrib_infos.end()) {
            return 0;
        }
        return &it2->second;
    }
    attrib_id get_attrib_id(const char* name) const {
        const auto it = attrib_indices.find(name);
        if(it == attrib_indices.end()) {
            return -1;
        }
        return it->second;
    }
    std::unordered_map<std::string, std::vector<attrib_id>>& getTable() {
        return table;
    }
};

inline EcsAttribTypeLib& getEcsAttribTypeLib() {
    static EcsAttribTypeLib lib;
    return lib;
}

template<typename T>
void regEcsAttrib(const char* name, const char* category = "") {
    getEcsAttribTypeLib().add<T>(category, name);
}


#endif
