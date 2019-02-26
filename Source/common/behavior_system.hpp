#ifndef BEHAVIOR_SYSTEM_HPP
#define BEHAVIOR_SYSTEM_HPP

#include "scene.hpp"

#include <set>

class Updatable : public Component {
    RTTR_ENABLE(Component)
public:
    virtual void init() {}
    virtual void update() = 0;
    virtual void cleanup() {}
};

class SysBehavior :
public ISceneProbe<Updatable> {
public:
    virtual void onCreateComponent(Updatable* u) {
        LOG("Updatable added");
        updatables.insert(u);
        u->init();
    }
    virtual void onRemoveComponent(Updatable* u) {
        LOG("Updatable removed");
        updatables.erase(u);
        u->cleanup();
    }

    void setScene(Scene* scn) {
        if(scene) {
            scene->removeProbe<Updatable>();
        }
        scene = scn;
        if(scene) {
            scene->setProbe<Updatable>(this);
        }
    }

    void update() {
        for(auto u : updatables) {
            u->update();
        }
    }
private:
    Scene* scene = 0;
    std::set<Updatable*> updatables;
};

#endif
