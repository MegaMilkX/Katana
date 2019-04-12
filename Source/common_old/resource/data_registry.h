#ifndef DATA_REGISTRY_H
#define DATA_REGISTRY_H

#include "data_source.h"
#include <unordered_map>
#include <map>
#include <cctype>
#include "../util/log.hpp"
#include "../util/filesystem.hpp"
#include "../util/has_suffix.hpp"

class DataRegistry {
public:
    typedef std::map<std::string, DataSourceRef> DataSourceMap_t;
    
    void ScanArchives();
    void ScanFilesystem(const std::string& rootDir);

    void Clear() {
        dataSources.clear();
        name_list.clear();
    }

    DataSourceRef Get(const std::string& n) {
        std::string name = n;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
        }
        if(dataSources.count(name) == 0) return 0;
        return dataSources[name];
    }
    size_t Count() const { return dataSources.size(); }
    DataSourceRef GetById(size_t id) {
        auto it = dataSources.begin();
        std::advance(it, id);
        if(it == dataSources.end()) return 0;
        return it->second;
    }
    std::string GetNameById(size_t id) {
        auto it = dataSources.begin();
        std::advance(it, id);
        if(it == dataSources.end()) return "";
        return it->first;
    }

    void Add(const std::string& n, DataSourceRef dataSource) {
        std::string name = n;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
            if(name[i] == '\\') {
                name[i] = '/';
            }
        }
        if(!dataSources.count(name)) {
            name_list.emplace_back(name);
        }
        dataSources[name] = dataSource;
        
        LOG("Added data source '" << n << "'");
    }

    const std::vector<std::string>& getList() const {
        return name_list;
    }
    std::vector<std::string> makeList(const std::string& prefix) const {
        std::vector<std::string> names;
        for(auto n : name_list) {
            if(has_suffix(n, prefix)) {
                names.emplace_back(n);
            }
        }
        return names;
    }
private:
    DataSourceMap_t dataSources;
    std::vector<std::string> name_list;
};

inline DataRegistry& GlobalDataRegistry() {
    static DataRegistry reg;
    return reg;
}

inline void registerGlobalFileSource(const std::string& fname) {
    GlobalDataRegistry().Add(
        fname,
        DataSourceRef(new DataSourceFilesystem(get_module_dir() + "\\" + fname))
    );
}

#endif
