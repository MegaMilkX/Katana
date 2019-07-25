#ifndef OBJECT_SET_HPP
#define OBJECT_SET_HPP

#include <set>

#include "../common/scene/game_object.hpp"

struct ObjectSet {
    std::set<GameObject*> objects;
public:
    bool empty() { return objects.empty(); }
    void clear() { objects.clear(); }
    bool contains(GameObject* o) { return objects.find(o) != objects.end(); }
    void add(GameObject* o) { objects.insert(o); }
    void clearAndAdd(GameObject* o) { clear(); add(o); }
    const std::set<GameObject*>& getAll() { return objects; }
};

#endif
