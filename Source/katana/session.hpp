#ifndef SESSION_HPP
#define SESSION_HPP

#include "actor.hpp"
#include "../common/scene/game_scene.hpp"

#include <vector>
#include <set>

class ktSession {
    GameScene               scene;
    std::set<Actor*>        actors;
public:
    virtual ~ktSession() {
        clearActors();
    }

    GameScene*  getScene() { return &scene; }

    template<typename T>
    T*          createActor() {
        T* a = new T();
        actors.insert(a);
        a->session = this;
        return a;
    }
    void        removeActor(Actor* a) {
        actors.erase(a);
        delete a;
    }
    void        clearActors() {
        for(auto a : actors) {
            delete a;
        }
        actors.clear();
    }

    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onCleanup() {}

    void        start           () { onStart(); for(auto o : actors) { o->onInit(); } }
    void        update          () { onUpdate(); for(auto o : actors) { o->onUpdate(); } }
    void        stop            () { onCleanup(); for(auto o : actors) { o->onCleanup(); } }
};

#endif
