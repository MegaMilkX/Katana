#ifndef ECS_ATTRIB_LIB_HPP
#define ECS_ATTRIB_LIB_HPP

#include <unordered_map>

typedef int32_t attrib_id;

class ecsAttribBase;

struct attrib_type_info {
    typedef ecsAttribBase*(*constructor_fn_t)(void);
    typedef void          (*constructor_in_place_fn_t)(ecsAttribBase*);
    
    constructor_fn_t constructor;
    constructor_in_place_fn_t constructor_in_place;
    std::string name;
    size_t size_of = 0;
};

class EcsAttribTypeLib {
    std::unordered_map<std::string, std::vector<attrib_id>> table;
    std::unordered_map<attrib_id, attrib_type_info> attrib_infos;
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
        inf.name = name;
        inf.size_of = sizeof(T);
        table[category].emplace_back(T::get_id_static());
        attrib_infos[T::get_id_static()] = inf;
    }
    const attrib_type_info* get_info(attrib_id id) const {
        const auto it = attrib_infos.find(id);
        if(it == attrib_infos.end()) {
            return 0;
        }
        return &it->second;
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
