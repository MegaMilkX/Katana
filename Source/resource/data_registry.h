#ifndef DATA_REGISTRY_H
#define DATA_REGISTRY_H

#include "data_source.h"
#include <unordered_map>
#include <map>

class DataRegistry {
public:
    typedef std::map<std::string, DataSourceRef> DataSourceMap_t;
    
    void ScanArchives();
    void ScanFilesystem(const std::string& rootDir);

    void Clear() {
        dataSources.clear();
    }

    DataSourceRef Get(const std::string& name) {
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

    void Add(const std::string& name, DataSourceRef dataSource) {
        dataSources[name] = dataSource;
    }
private:
    DataSourceMap_t dataSources;
};

inline DataRegistry& GlobalDataRegistry() {
    static DataRegistry reg;
    return reg;
}

#endif
