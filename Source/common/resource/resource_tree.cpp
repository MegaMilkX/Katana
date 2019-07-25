#include "resource_tree.hpp"

#include <cctype>


ResourceTree gResourceTree;

static std::string sanitizeString(const std::string& str) {
    std::string name = str;
    for(size_t i = 0; i < name.size(); ++i) {
        name[i] = (std::tolower(name[i]));
        if(name[i] == '\\') {
            name[i] = '/';
        }
    }
    return name;
}


void ResourceNode::setDataSource(DataSource* src) {
    data_src.reset(src);
}

size_t ResourceNode::childCount() const {
    return nodes.size();
}

ResourceNode* ResourceNode::getChild(const std::string& name) {
    auto& ptr = nodes[name];
    if(!ptr) {
        ptr.reset(new ResourceNode);
        ptr->parent = this;
        ptr->name = name;
    }
    return ptr.get();
}
ResourceNode* ResourceNode::findChild(const std::string& name) {
    auto it = nodes.find(name);
    ResourceNode* r = 0;
    if(it != nodes.end()) {
        r = it->second.get();
    }
    return r;
}
const std::map<std::string, std::unique_ptr<ResourceNode>>& ResourceNode::getChildren() const {
    return nodes;
}
std::string ResourceNode::getName() const {
    return name;
}
std::string ResourceNode::getFullName() const {
    if(parent) {
        std::string pname = parent->getFullName();
        if(pname.size() > 0) {
            return pname + "/" + name;
        } else {
            return name;
        }
    } else {
        return name;
    }
}

bool ResourceNode::isLoaded() const {
    return (bool)resource;
}

void ResourceNode::print() {
    LOG(name);
    for(auto& kv : nodes) {
        kv.second->print();
    }
}


void ResourceTree::clear() {
    root = ResourceNode();
}

void ResourceTree::mapType(const std::string& mask, rttr::type type) {
    //type_map[mask] = type;
}

void ResourceTree::insert(const std::string& path, DataSource* data_src) {
    std::vector<std::string> tokens = split(path, '/');
    ResourceNode* cur_node = &root;
    for(int i = 0; i < tokens.size(); ++i) {
        cur_node = cur_node->getChild(tokens[i]);
    }
    cur_node->setDataSource(data_src);
}
ResourceNode* ResourceTree::get(const std::string& p) {
    std::string path = sanitizeString(p);
    std::vector<std::string> tokens = split(path, '/');

    ResourceNode* cur_node = tokens.size() > 0 ? &root : 0;
    for(int i = 0; i < tokens.size(); ++i) {
        if(!cur_node) break;
        cur_node = cur_node->findChild(tokens[i]);
    }
    if(cur_node == 0) {
        LOG_WARN("No such resource node: '" << path << "'");
    }
    return cur_node;
}
ResourceNode* ResourceTree::getRoot() { 
    return &root; 
}

void ResourceTree::print() {
    root.print();
}

void ResourceTree::scanFilesystem(const std::string& rootDir) {
    ResourceTree& resTree = *this;
    root = ResourceNode();
    
    std::vector<std::string> files = find_all_files(rootDir, "*.*");
    std::vector<std::string> files_relative = files;
    for(int i = 0; i < files.size(); ++i) {
        auto& f = files_relative[i];
        f.erase(f.find_first_of(rootDir), rootDir.size());
        if(f[0] == '\\') f.erase(0, 1);
        std::replace(f.begin(), f.end(), '\\', '/');
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);

        resTree.insert(f, new DataSourceFilesystem(files[i]));
    }
}
