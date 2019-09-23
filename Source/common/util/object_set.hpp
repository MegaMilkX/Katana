#ifndef OBJECT_SET_HPP
#define OBJECT_SET_HPP

#include <set>

class ktNode;
struct ObjectSet {
    std::set<ktNode*> objects;
public:
    bool empty() { return objects.empty(); }
    void clear() { objects.clear(); }
    bool contains(ktNode* o) { return objects.find(o) != objects.end(); }
    void add(ktNode* o) { objects.insert(o); }
    void clearAndAdd(ktNode* o) { clear(); add(o); }
    const std::set<ktNode*>& getAll() { return objects; }
};

#endif
