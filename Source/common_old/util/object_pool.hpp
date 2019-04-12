#ifndef OBJECT_POOL_HPP
#define OBJECT_POOL_HPP

#include <vector>
#include <set>

template<typename T>
class ObjectPool {
public:
    size_t acquire() {
        size_t i;
        if(!free_slots.empty()) {
            i = *free_slots.begin();
            free_slots.erase(free_slots.begin());
        } else {
            i = objects.size();
            objects.emplace_back(T());
        }
        return i;
    }
    void free(size_t i) {
        free_slots.insert(i);
    }
    T* deref(size_t i) {
        return &objects[i];
    }
private:
    std::vector<T> objects;
    std::set<size_t> free_slots;
};

#endif
