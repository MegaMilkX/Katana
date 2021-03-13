#ifndef KT_TRANSFORM_HPP
#define KT_TRANSFORM_HPP

#include <assert.h>
#include "../gfxm.hpp"

class ktTransform {
    ktTransform* parent = 0;
    ktTransform* next_sibling = 0;
    ktTransform* first_child = 0;

    gfxm::vec3 t;
    gfxm::quat r;
    gfxm::vec3 s;
    gfxm::mat4 world;

    void addChild(ktTransform* child) {
        ktTransform* ch = first_child;
        while(ch != 0) {
            ch = ch->next_sibling;
        }
        ch = child;
        ch->next_sibling = 0;
    }
    void removeChild(ktTransform* child) {
        auto ch = first_child;
        while(ch != 0 && ch != child) {
            ch = ch->next_sibling;
        }
    }
public:
    ktTransform() {
        
    }
    ~ktTransform() {
        if(parent) {
            detach();
        }
    }

    void attach(ktTransform* parent) {
        if(this->parent) {
            detach();
        }
        this->parent = parent;
        parent->addChild(this);
    }
    void detach() {
        assert(parent);
        parent->removeChild(this);
        parent = 0;
    }
};


#endif
