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
#include "../platform/platform.hpp"

class ResourceNode {
    ResourceNode* parent = 0;
    std::string name;
    std::map<std::string, std::shared_ptr<ResourceNode>> nodes;
    std::shared_ptr<DataSource> data_src;
    std::shared_ptr<Resource> resource;
public:
    ResourceNode();
    ResourceNode(const std::string& name);
    // ResourceNode claims ownership of the DataSource ptr
    void                            setDataSource(DataSource* src);
    void                            setPointer(std::shared_ptr<Resource> r);
    size_t                          childCount() const;
    std::shared_ptr<ResourceNode>   getChild(const std::string& name);
    std::shared_ptr<ResourceNode>   findChild(const std::string& name);
    const std::map<std::string, std::shared_ptr<ResourceNode>>& 
                                    getChildren() const;
    std::string                     getName() const;
    std::string                     getFullName() const;
    std::shared_ptr<DataSource>     getSource();
    Resource*                       getPointer();
    template<typename T>
    std::shared_ptr<T>              createResource();
    template<typename T>
    std::shared_ptr<T>              getResource();
    template<typename T>
    void                            overrideResource(std::shared_ptr<T> r);

    bool                            isLoaded() const;
    void                            tryRelease();
    void                            tryReleaseRecursive();

    // Remove nodes that don't exist in the other tree
    void                            difference(ResourceNode* other);
    // Add only new nodes
    void                            add(ResourceNode* other);

    void                            print();
};

template<typename T>
std::shared_ptr<T> ResourceNode::createResource() {
    assert(!resource);
    resource = std::shared_ptr<T>(new T);
    return std::dynamic_pointer_cast<T>(resource);
}
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
    ResourceNode*                   insert(const std::string& path, DataSource* data_src);
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
std::shared_ptr<T> retrieve(const std::string& path, bool create_empty_if_doesnt_exist = false) {
    ResourceNode* n = gResourceTree.get(path);
    if(!n) {
        if(create_empty_if_doesnt_exist) {
            n = gResourceTree.insert(path, new DataSourceFilesystem(get_module_dir()+"/"+platformGetConfig().data_dir+"/"+path));
            if(!n->getPointer()) {
                return n->createResource<T>();
            } else {
                return n->getResource<T>();
            }
            
        }
        return 0;
    }
    return n->getResource<T>();
}

template<typename T>
std::shared_ptr<T> getResource(const std::string& path, bool create_empty_if_doesnt_exist = false) {
    return retrieve<T>(path, create_empty_if_doesnt_exist);
}

// Only for tools
bool overwriteResourceFile(std::shared_ptr<Resource> r);


#endif