#ifndef RESOURCE_FACTORY_H
#define RESOURCE_FACTORY_H

#include "resource.h"
#include "data_registry.h"
#include <unordered_map>
#include "../util/log.hpp"

class ResourceFactory {
public:
    typedef std::weak_ptr<Resource> res_weak_ptr_t;
    typedef std::shared_ptr<Resource> res_shared_ptr_t;
    typedef std::unordered_map<std::string, res_weak_ptr_t> resource_map_t;
    
    template<typename T>
    std::shared_ptr<T> Get(const std::string& n) {
        std::string name = sanitizeString(n);

        DataSourceRef dataSrc = GlobalDataRegistry().Get(name);
        if(!dataSrc) {
            LOG_WARN("Data source '" << name << "' doesn't exist.");
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> ptr;
        res_weak_ptr_t& weak = resources[name];
        if(weak.expired()) {
            std::shared_ptr<base_stream> strm = dataSrc->make_stream();

            ptr.reset(new T());
            ptr->Name(name);
            ptr->Storage(Resource::GLOBAL);
            if(!ptr->deserialize(*strm.get(), strm->size())) {
                LOG_WARN("Failed to build resource " << name);
                return std::shared_ptr<T>();
            }
            weak = ptr;
        } else {
            ptr = std::dynamic_pointer_cast<T>(weak.lock());
        }
        return ptr;
    }

    void forceReload(const std::string& name) {
        auto& it = resources.find(name);
        if(it == resources.end()) {
            LOG_WARN("Force reload failed: no resource to reload: " << name);
            return;
        }
        auto ptr = it->second.lock();
        if(!ptr) {
            LOG_WARN("Force reload failed: resource expired: " << name);
            return;
        }
        DataSourceRef data_src = GlobalDataRegistry().Get(ptr->Name());
        if(!data_src) {
            LOG_WARN("Force reload failed: no data source '" << ptr->Name() << "'");
            return;
        }

        std::shared_ptr<base_stream> strm = data_src->make_stream();
        if(!ptr->deserialize(*strm.get(), strm->size())) {
            LOG_WARN("Force reload failed: deserialize func failed: " << name);
            return;
        }
    }
private:
    std::string sanitizeString(const std::string& str) {
        std::string name = str;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
            if(name[i] == '\\') {
                name[i] = '/';
            }
        }
        return name;
    }

    resource_map_t resources;
};

inline ResourceFactory& GlobalResourceFactory() {
    static ResourceFactory f;
    return f;
}

template<typename T>
std::shared_ptr<T> getResource(const std::string& name) {
    return GlobalResourceFactory().Get<T>(name);
}

template<typename T>
std::shared_ptr<T> retrieve(const std::string& name) {
    return getResource<T>(name);
}

#endif
