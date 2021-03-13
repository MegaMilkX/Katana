#ifndef KT_TRANSFORM_2_HPP
#define KT_TRANSFORM_2_HPP

#include <stdint.h>
#include <gfxm.hpp>

static const uint32_t INVALID_TRANSFORM_INDEX = -1;
static const uint32_t INVALID_TRANSFORM_VERSION = -1;

class TransformData;
class hTransform {
    union {
        struct {
            uint32_t index;
            uint32_t version;
        };
        uint64_t handle;
    };
public:
    hTransform()
    : index(INVALID_TRANSFORM_INDEX), version(INVALID_TRANSFORM_VERSION) {}
    hTransform(uint32_t index, uint32_t version)
    : index(index), version(version) {}

    TransformData* operator->();
    const TransformData* operator->() const;

    operator uint64_t () const {
        return handle;
    }
    operator bool () const;

    bool isValid() const { return operator bool(); }

    uint32_t getIndex() const { return index; }
    uint32_t getVersion() const { return version; }
};


#include <forward_list>
#include "dirty_array.hpp"

class TransformData {
    hTransform parent;
    TransformData* next_sibling = 0;
    TransformData* first_child  = 0;

    gfxm::vec3 t = gfxm::vec3(0, 0, 0);
    gfxm::quat r = gfxm::quat(0, 0, 0, 1);
    gfxm::vec3 s = gfxm::vec3(1, 1, 1);
    gfxm::mat4 world = gfxm::mat4(1);

    std::forward_list<ktDirtyArrayElemRef> dirty_array_elements;

    void dirty() {
        // TODO: check if already dirty
        // Haven't decided yet if should use a bool flag or dirty array.
        //      - Bool will make it possible to repeatedly set and clear the flag on same frame, causing checks for
        //      dirty array elements each time. Which is not too bad, since those will not be moved if already flagged
        //      - Dirty array doesnt allow to update on getWorldTransform()
        //      Also it will require single update for all worlds, since there's only one TransformData array for the entire program
        //          - But the dirty array for transforms should only keep handles to transforms, no use moving whole data around on change
        //          - It's also an option to keep an array of hTransforms for each world
        //          There's not much point in keeping TransformData by value in a dirty array,
        //          such an approach will complicate handles
        for(auto& e : dirty_array_elements) {
            e.setDirty();
        }
        TransformData* child = first_child;
        while(child) {
            child->dirty();
            child = child->next_sibling;
        }
    }
public:
    void linkDirtyArray(ktDirtyArrayBase* array, ktDirtyArrayElement* elem) {
        ktDirtyArrayElemRef ref;
        ref.array = array;
        ref.elem = elem;
        dirty_array_elements.push_front(ref);
    }
    void unlinkDirtyArray(ktDirtyArrayBase* array, ktDirtyArrayElement* elem) {
        auto prev = dirty_array_elements.before_begin();
        auto it   = dirty_array_elements.begin();
        while(it != dirty_array_elements.end()) {
            if(it->array == array && it->elem == elem) {
                dirty_array_elements.erase_after(prev);
                break;
            }
            prev = it;
            ++it;
        }
    }

    void setParent(hTransform p) { 
        parent = p;

        TransformData*& child = parent->first_child;
        while(child) {
            child = child->next_sibling;
        }
        child = this;

        dirty(); 
    }
    void clearParent() {
        if(!parent) {
            return;
        }
        if(parent->first_child == this) {
            parent->first_child = 0;
        } else {
            TransformData*& child = parent->first_child;
            while(child) {
                if(child->next_sibling == this) {
                    child->next_sibling = child->next_sibling->next_sibling;
                    break;
                }
                child = child->next_sibling;
            }
        }
        
        parent = hTransform();
        dirty();
    }

    void setTranslation(const gfxm::vec3& t) { this->t = t; dirty(); }
    void setRotation(const gfxm::quat& q) { this->r = q; dirty(); }
    void setRotationEuler(const gfxm::vec3& euler) { r = gfxm::euler_to_quat(euler); dirty(); }
    void setScale(const gfxm::vec3& s) { this->s = s; dirty(); }
    void setScale(float s) { this->s = gfxm::vec3(s, s, s); dirty(); }

    void translate(const gfxm::vec3& delta) { this->t += delta; dirty(); }
    void rotate(float angle, const gfxm::vec3& axis) { r = gfxm::angle_axis(angle, axis) * r; dirty(); }
    void rotate(const gfxm::quat& q) { r = q * r; dirty(); }
    void rotateEuler(const gfxm::vec3& euler) { r = gfxm::euler_to_quat(euler) * r; dirty(); }

    const gfxm::vec3& getLclTranslation() const { return t; }
    const gfxm::quat& getLclRotation() const { return r; }
    const gfxm::vec3& getLclScale() const { return s; }

    // TODO: lookAt
    // TODO: getters for back, right, up, etc.
    // TODO: dirty flag or smth

    gfxm::mat4 getLocalTransform() {
        return gfxm::translate(gfxm::mat4(1.0f), t)
            * gfxm::to_mat4(r)
            * gfxm::scale(gfxm::mat4(1.0f), s);
    }
    const gfxm::mat4& getWorldTransform() {
        if(!parent) {
            return world = getLocalTransform();
        } else {
            return world = parent->getWorldTransform() * getLocalTransform();
        }
    }
};

hTransform createTransformNode();
void destroyTransformNode(hTransform hdl);

class TransformNode_ {
    hTransform handle;
public:
    TransformNode_() {
        handle = createTransformNode();
    }
    ~TransformNode_() {
        destroyTransformNode(handle);
    }

    hTransform getHandle() { return handle; }

    TransformData* operator->() {
        return handle.operator->();
    }
    const TransformData* operator->() const {
        return handle.operator->();
    }
};

#endif
