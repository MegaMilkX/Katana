#ifndef SCENE_NODE_HPP
#define SCENE_NODE_HPP


#include <string>


class SceneNode {
    SceneNode*          parent      = 0;
    std::string         name        = "Node";
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
};

#endif
