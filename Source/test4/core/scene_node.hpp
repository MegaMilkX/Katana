#ifndef SCENE_NODE_HPP
#define SCENE_NODE_HPP


#include <string>
#include "../../common/resource/mesh.hpp"

class Attribute {
public:
    virtual ~Attribute() {}
};

class attrModel : public Attribute {
public:
    std::shared_ptr<Mesh> mesh;
};


class SceneNode {
    typedef std::map<rttr::type, std::shared_ptr<Attribute>> attrib_map_t;
    typedef std::set<std::shared_ptr<SceneNode>>             node_set_t;

    SceneNode*          parent      = 0;
    std::string         name        = "Node";
    attrib_map_t        attribs;
    node_set_t          children;

public:
    const std::string&  getName     () const { return name; }
    void                setName     (const std::string& n) { name = n; }
    SceneNode*          getParent   () { return parent; }
    SceneNode*          getRoot     () {
        SceneNode* r = this;
        SceneNode* p = getParent();
        while(p != 0) {
            r = p;
            p = p->getParent();
        }
        return r;
    }

    SceneNode*          createChild() {
        std::shared_ptr<SceneNode> node(new SceneNode());
        node->parent = this;
        children.insert(node);
        return node.get();
    }
    uint64_t            childCount() {
        return children.size();
    }
    SceneNode*          getChild(uint64_t i) {
        auto it = children.begin();
        std::advance(it, i);
        if(it == children.end()) {
            return 0;
        } else {
            return (*it).get();
        }
    }

    template<typename T>
    T*                  get() {
        std::shared_ptr<Attribute> attrib(new T());
        attribs[rttr::type::get<T>()] = attrib;
        return (T*)attrib.get();
    }
    template<typename T>
    T*                  find() {
        auto it = attribs.find(rttr::type::get<T>());
        if(it == attribs.end()) {
            return 0;
        }
        return (T*)it->second.get();
    }
};

#endif
