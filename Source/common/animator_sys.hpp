#ifndef ANIMATOR_SYS_HPP
#define ANIMATOR_SYS_HPP

#include "scene.hpp"
#include "animator.hpp"

#include <set>

class AnimatorSys : 
public ISceneProbe<Animator> {
public:
    virtual void onCreateComponent(Animator* a) {
        LOG("Animator added");
        animators.insert(a);
    }
    virtual void onRemoveComponent(Animator* a) {
        LOG("Animator removed");
        animators.erase(a);
    }

    void setScene(Scene* scn) {
        if(scene) {
            scene->removeProbe<Animator>();
        }
        scene = scn;
        if(scene) {
            scene->setProbe<Animator>(this);
        }
    }

    void Update(float dt) {
        for(auto a : animators) {
            a->Update(dt);
        }
    }
private:
    Scene* scene = 0;
    std::set<Animator*> animators;
};

#endif
