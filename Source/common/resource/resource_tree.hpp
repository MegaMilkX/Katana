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
    std::map<std::string, std::shared_ptr<ResourceNode>> nodes;
    std::shared_ptr<DataSource> data_src;
    std::shared_ptr<Resource> resource;
public:
    // ResourceNode claims ownership of the DataSource ptr
    void                            setDataSource(DataSource* src);
    size_t                          childCount() const;
    std::shared_ptr<ResourceNode>   getChild(const std::string& name);
    std::shared_ptr<ResourceNode>   findChild(const std::string& name);
    const std::map<std::string, std::shared_ptr<ResourceNode>>& 
                                    getChildren() const;
    std::string                     getName() const;
    std::string                     getFullName() const;
    std::shared_ptr<DataSource>     getSource();
    template<typename T>
    std::shared_ptr<T>              getResource();
    template<typename T>
    void                            overrideResource(std::shared_ptr<T> r);

    bool                            isLoaded() const;

    // Remove nodes that don't exist in the other tree
    void                            difference(ResourceNode* other);
    // Add only new nodes
    void                            add(ResourceNode* other);

    void                            print();
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
template<typename T>
void ResourceNode::overrideResource(std::shared_ptr<T> r) {
    resource = std::dynamic_pointer_cast<Resource>(r);
}


class ResourceTree {
    std::shared_ptr<ResourceNode> root;
    
public:
    ResourceTree();

    void                            clear();
    void                            mapType(const std::string& mask, rttr::type type);
    void                            insert(const std::string& path, DataSource* data_src);
    ResourceNode*                   get(const std::string& path);
    std::weak_ptr<ResourceNode>     get_weak(const std::string& path);
    std::shared_ptr<ResourceNode>&  getRoot();
    std::shared_ptr<ResourceNode>   find_shared(const std::string& path);

    // Add new nodes, remove absent ones
    void            update(ResourceTree& other);

    void            scanFilesystem(const std::string& rootDir);

    void            print();

private:
    
};

extern ResourceTree gResourceTree;

template<typename T>
std::shared_ptr<T> retrieve(const std::string& path) {
    ResourceNode* n = gResourceTree.get(path);
    if(!n) return 0;
    return n->getResource<T>();
}


#endif