#ifndef KT_DIRTY_ARRAY_HPP
#define KT_DIRTY_ARRAY_HPP

#include <assert.h>
#include <vector>
#include <stdint.h>

struct ktDirtyArrayElement {
    uint64_t dirty_index;
};

class ktDirtyArrayBase {
protected:
    std::vector<ktDirtyArrayElement*>   array;
    uint64_t                            dirty_count;

    void swap(ktDirtyArrayElement* a, ktDirtyArrayElement* b) {
        auto ia = a->dirty_index;
        auto ib = b->dirty_index;
        a->dirty_index = ib;
        b->dirty_index = ia;
        array[ia] = b;
        array[ib] = a;
    }
public:
    virtual ~ktDirtyArrayBase() {}

    uint64_t dirtyCount() const { return dirty_count; }
    void clearDirtyCount() { dirty_count = 0; }

    void add(ktDirtyArrayElement* elem) {
        elem->dirty_index = array.size();
        array.push_back(elem);
        // Immediately mark as dirty
        setDirty(elem);
    }
    void remove(ktDirtyArrayElement* elem) {
        auto last_dirty = dirty_count - 1;
        if(elem->dirty_index > last_dirty) {
            swap(elem, array[array.size() - 1]);
        } else {
            // Move out of the dirty section first
            swap(elem, array[last_dirty]);
            // Then to the end of the array
            swap(elem, array[array.size() - 1]);
            dirty_count--;
        }

        array.resize(array.size() - 1);
    }

    void setDirty(ktDirtyArrayElement* elem) {
        if(elem->dirty_index < dirty_count) {
            return;
        }
        swap(elem, array[dirty_count]);
        ++dirty_count;
    }
};

template<typename T>
class ktDirtyArray : public ktDirtyArrayBase {
public:
    const T* operator[](size_t idx) const {
        return (const T*)array[idx];
    }
    T* operator[](size_t idx) {
        return (T*)array[idx];
    }
};

struct ktDirtyArrayElemRef {
    ktDirtyArrayElement*    elem;
    ktDirtyArrayBase*       array;

    void setDirty() {
        array->setDirty(elem);
    }
};


#endif
