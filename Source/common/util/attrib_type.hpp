#ifndef ATTRIB_TYPE_HPP
#define ATTRIB_TYPE_HPP

#include <string>
#include <vector>
#include <assert.h>

typedef int32_t attrib_id;

struct attrib_type_data {
    std::string name;
    size_t size_of;
};

class AttribTypeStorage {
    std::vector<attrib_type_data> type_data;
    std::unordered_map<std::string, attrib_id> name_to_id;
    void expand() {
        type_data.resize(type_data.size() + 1);
    }
public:
    attrib_id next_attrib_id() {
        attrib_id id = type_data.size();
        expand();
        return id;
    }
    attrib_type_data& get_type_data(attrib_id id) {
        return type_data[id];
    }
    void set_name(attrib_id id, const char* name) {
        name_to_id[name] = id;
        type_data[id].name = name;
    }

    const char* get_name(attrib_id id) const {
        assert(id < type_data.size());
        return type_data[id].name.c_str();
    }
    attrib_id get_id(const char* name) const {
        auto it = name_to_id.find(name);
        if(it == name_to_id.end()) {
            return std::numeric_limits<attrib_id>::max();
        }
        return it->second;
    }
};

inline AttribTypeStorage& get_attrib_type_storage() {
    static AttribTypeStorage storage;
    return storage;
}

template<typename T>
attrib_id get_attrib_id() {
    static attrib_id id = get_attrib_type_storage().next_attrib_id();
    return id;
}

class attrib_type {
    attrib_id id;

    template<typename T>
    attrib_id create_type_data() {
        attrib_id id = get_attrib_id<T>();
        attrib_type_data& data = get_attrib_type_storage().get_type_data(id);
        data.name = "UNKNOWN";
        data.size_of = sizeof(T):
        return id;
    }

    template<typename T>
    attrib_type create_or_get_type() {
        static const attrib_type t = attrib_type(get_attrib_type_storage().create_type_data<T>());
        return t;
    }
public:
    attrib_type() {}
    attrib_type(attrib_id id) : id(id) {}

    attrib_id get_id() const {
        return id;
    }

    template<typename T>
    static attrib_type get() {
        using non_ref_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
        return create_or_get_type<non_ref_type>();
    }
    static attrib_type get(const char* name) {
        return attrib_type(get_attrib_type_storage().get_id(name));
    }

    const char* get_name() const {
        return get_attrib_type_storage().get_name(id);
    }
};

namespace std {
template <>
struct hash<attrib_type> {
    std::size_t operator()(const attrib_type& k) const {
        return hash<attrib_id>()(k.get_id());
    }
};
}


#endif
