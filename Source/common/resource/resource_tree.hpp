#ifndef RESOURCE_TREE_HPP
#define RESOURCE_TREE_HPP

#include <algorithm>
#include <string>
#include <map>
#include <cmath>

//#include <rttr/type>

#include "../util/filesystem.hpp"
#include "../util/split.hpp"
#include "../util/log.hpp"

#include "data_source.h"
#include "resource.h"

class ResourceNode {
    ResourceNode* parent = 0;
    std::string name;
    std::map<std::string, std::unique_ptr<ResourceNode>> nodes;
    std::unique_ptr<DataSource> data_src;
    std::shared_ptr<Resource> resource;
public:
    // ResourceNode claims ownership of the DataSource ptr
    void                        setDataSource(DataSource* src);
    size_t                      childCount() const;
    ResourceNode*               getChild(const std::string& name);
    ResourceNode*               findChild(const std::string& name);
    const std::map<std::string, std::unique_ptr<ResourceNode>>& 
                                getChildren() const;
    std::string                 getName() const;
    std::string                 getFullName() const;
    template<typename T>
    std::shared_ptr<T>          getResource();

    bool                        isLoaded() const;

    void                        print();
};

template<typename T>
std::shared_ptr<T> ResourceNode::getResource() {
    if(resource) return std::dynamic_pointer_cast<T>(resource);
    
    if(!data_src) return 0;
    resource.reset(new T());
    resource->Name(getFullName());
    resource->Storage(Resource::GLOBAL);
    auto strm = data_src->make_stream();
    if(!resource->deserialize(*strm.get(), strm->size())) {
        LOG_WARN("Failed to build resource " << resource->Name());
        resource.reset();
    }
    return (std::dynamic_pointer_cast<T>(resource));
}


class ResourceTree {
    ResourceNode root;
    //std::map<std::string, rttr::type> type_map;
public:
    void            clear();
    void            mapType(const std::string& mask, rttr::type type);
    void            insert(const std::string& path, DataSource* data_src);
    ResourceNode*   get(const std::string& path);
    ResourceNode*   getRoot();

    void            scanFilesystem(const std::string& rootDir);

    void            print();
};

extern ResourceTree gResourceTree;

template<typename T>
std::shared_ptr<T> retrieve(const std::string& path) {
    ResourceNode* n = gResourceTree.get(path);
    if(!n) return 0;
    return n->getResource<T>();
}


#endif