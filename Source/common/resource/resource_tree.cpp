#include "resource_tree.hpp"

#include <cctype>

#include "data_source.h"

#include "../platform/platform.hpp"

#include "../util/filesystem.hpp"


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


ResourceNode::ResourceNode() {

}
ResourceNode::ResourceNode(const std::string& name) {
    this->name = name;
    this->ext = get_extension(name);
    LOG("name: " << this->name << ", ext: " << this->ext);
}

void ResourceNode::setDataSource(DataSource* src) {
    data_src.reset(src);
}

void ResourceNode::setPointer(std::shared_ptr<Resource> r) {
    resource = r;
}

size_t ResourceNode::childCount() const {
    return nodes.size();
}

std::shared_ptr<ResourceNode> ResourceNode::getChild(const std::string& name) {
    auto& ptr = nodes[name];
    if(!ptr) {
        ptr.reset(new ResourceNode(name));
        ptr->parent = this;
    }
    return ptr;
}
std::shared_ptr<ResourceNode> ResourceNode::findChild(const std::string& name) {
    auto it = nodes.find(name);
    std::shared_ptr<ResourceNode> r;
    if(it != nodes.end()) {
        r = it->second;
    }
    return r;
}
const std::map<std::string, std::shared_ptr<ResourceNode>>& ResourceNode::getChildren() const {
    return nodes;
}
std::string ResourceNode::getName() const {
    return name;
}
const std::string& ResourceNode::getExtension() const {
    return ext;
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

std::shared_ptr<DataSource> ResourceNode::getSource() {
    return data_src;
}

Resource* ResourceNode::getPointer() {
    return resource.get();
}

std::vector<uint8_t> ResourceNode::readBytes() {
    std::vector<uint8_t> buf;
    if(!data_src) {
        return buf;
    }
    buf.resize(data_src->Size());
    data_src->ReadAll((char*)buf.data());
    return buf;
}

bool ResourceNode::isLoaded() const {
    return (bool)resource;
}

void ResourceNode::tryRelease() {
    if(resource.use_count() == 1) {
        resource.reset();
    }
}

void ResourceNode::tryReleaseRecursive() {
    for(auto& kv : nodes) {
        kv.second->tryReleaseRecursive();
    }
    tryRelease();
}


void ResourceNode::difference(ResourceNode* other) {
    std::set<std::string> to_delete;
    for(auto& n : nodes) {
        auto& it = other->nodes.find(n.first);
        if(it == other->nodes.end()) {
            to_delete.insert(n.first);
        } else {
            n.second->difference(it->second.get());
        }
    }

    for(auto& n : to_delete) {
        nodes.erase(n);
    }
}
void ResourceNode::add(ResourceNode* other) {
    std::set<ResourceNode*> new_nodes;
    for(auto& other_kv : other->nodes) {
        auto& this_kv = nodes.find(other_kv.first);
        if(this_kv != nodes.end()) {
            this_kv->second->add(other_kv.second.get());
        } else {
            ResourceNode* new_n = new ResourceNode(other_kv.second->name);
            new_n->parent = this;
            new_n->data_src = other_kv.second->data_src;
            new_nodes.insert(new_n);
        }
    }

    for(auto n : new_nodes) {
        nodes[n->name].reset(n);
        n->add(other->nodes[n->name].get());
    }
}

void ResourceNode::print() {
    LOG(name);
    for(auto& kv : nodes) {
        kv.second->print();
    }
}


ResourceTree::ResourceTree() {
    root.reset(new ResourceNode(""));
}

void ResourceTree::clear() {
    root.reset(new ResourceNode(""));
}

void ResourceTree::mapType(const std::string& mask, rttr::type type) {
    //type_map[mask] = type;
}

ResourceNode* ResourceTree::insert(const std::string& path, DataSource* data_src) {
    std::vector<std::string> tokens = split(path, '/');
    ResourceNode* cur_node = root.get();
    for(int i = 0; i < tokens.size(); ++i) {
        cur_node = cur_node->getChild(tokens[i]).get();
    }
    cur_node->setDataSource(data_src);
    return cur_node;
}
ResourceNode* ResourceTree::get(const std::string& p) {
    return get_weak(p).lock().get();
}
std::weak_ptr<ResourceNode> ResourceTree::get_weak(const std::string& p) {
    std::string path = sanitizeString(p);
    std::vector<std::string> tokens = split(path, '/');

    std::shared_ptr<ResourceNode> cur_node = tokens.size() > 0 ? root : 0;
    for(int i = 0; i < tokens.size(); ++i) {
        if(!cur_node) break;
        cur_node = cur_node->findChild(tokens[i]);
    }
    if(!cur_node) {
        LOG_WARN("No such resource node: '" << path << "'");
    }
    return cur_node;
}

std::shared_ptr<ResourceNode>& ResourceTree::getRoot() { 
    return root; 
}

std::shared_ptr<ResourceNode> ResourceTree::find_shared(const std::string& p) {
    std::string path = sanitizeString(p);
    std::vector<std::string> tokens = split(path, '/');

    std::shared_ptr<ResourceNode> cur_node = tokens.size() > 0 ? root : 0;
    for(int i = 0; i < tokens.size(); ++i) {
        if(!cur_node) break;
        cur_node = cur_node->findChild(tokens[i]);
    }
    if(!cur_node) {
        LOG_WARN("No such resource node: '" << path << "'");
    }
    return cur_node;
}

void ResourceTree::update(ResourceTree& other) {
    root->difference(other.root.get());
    root->add(other.root.get());
}

void ResourceTree::print() {
    root->print();
}

void ResourceTree::scanFilesystem(const std::string& rootDir) {
    ResourceTree& resTree = *this;
    ResourceTree new_tree;
    
    std::vector<std::string> files = find_all_files(rootDir, "*.*");
    std::vector<std::string> files_relative = files;
    for(int i = 0; i < files.size(); ++i) {
        auto& f = files_relative[i];
        f.erase(f.find_first_of(rootDir), rootDir.size());
        if(f[0] == '\\') f.erase(0, 1);
        std::replace(f.begin(), f.end(), '\\', '/');
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);

        new_tree.insert(f, new DataSourceFilesystem(files[i]));
    }

    resTree.update(new_tree);
}


bool overwriteResourceFile(std::shared_ptr<Resource> r) {
    return r->write_to_file(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + r->Name());
}