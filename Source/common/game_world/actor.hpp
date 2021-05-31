#ifndef KT_ACTOR_HPP
#define KT_ACTOR_HPP

#include "../ecs/world.hpp"
#include "../draw_list.hpp"

class ktGameWorld;
// Base class for most game objects or entities
// You have to inherit it if you want your objects/actors to be serialized within a level
class ktActor {
    RTTR_ENABLE()
    
    std::string name = "Actor";
    ecsEntityHandle h_entity;
    ktGameWorld* world = 0;

    gfxm::vec3 translation = gfxm::vec3(0,0,0);
    gfxm::quat rotation = gfxm::quat(0,0,0,1);
    gfxm::vec3 scale = gfxm::vec3(1,1,1);
    gfxm::mat4 world_transform = gfxm::mat4(1.0f);
public:
    void setName(const char* name) { this->name = name; }
    const char* getName() const { return name.c_str(); }

    void setTranslation(const gfxm::vec3& t) { translation = t; }
    void setRotation(const gfxm::quat& q) { rotation = q; }
    void setScale(const gfxm::vec3& s) { scale = s; }

    const gfxm::vec3& getTranslation() const { return translation; }
    const gfxm::quat& getRotation() const { return rotation; }
    const gfxm::vec3& getScale() const { return scale; }

    const gfxm::mat4& getWorldTransform() {
        return world_transform = 
            gfxm::translate(gfxm::mat4(1.0f), translation) * 
            gfxm::to_mat4(rotation) * 
            gfxm::scale(gfxm::mat4(1.0f), scale);
    }

    virtual void onSpawn(ktGameWorld* world) {}
    virtual void onDespawn(ktGameWorld* world) {}

    virtual void onUpdate(float dt) {}
    virtual void onPostCollisionUpdate(float dt) {}
    
};


#endif
