#ifndef KT_GAME_OBJECT_HPP
#define KT_GAME_OBJECT_HPP

#include <string>

#include "component/component.hpp"
#include "../archetype/archetype.hpp"

#include "entity.hpp"

class ktGameWorld;
class ktGameObject : public ktEntity {
    std::string name;

public:
    ktGameObject(ktGameWorld* world);
    ~ktGameObject();

    const std::string& getName() const { return name; }
    void               setName(const std::string& name) { this->name = name; }    

    virtual void onGui() = 0;
};


#endif
