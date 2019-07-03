#ifndef SESSION_HPP
#define SESSION_HPP

#include "actor.hpp"
#include "../../common/scene/game_scene.hpp"

#include <vector>
#include <set>

class GameSession {
    GameScene*              scene = 0;
    std::set<Actor*>        actors;
public:
    virtual ~GameSession() {
        clearActors();
    }

    void        setScene(GameScene* scn) {
        scene = scn;
    }
    GameScene*  getScene() {
        return scene;
    }

    template<typename T>
    T*          createActor() {
        T* a = new T();
        actors.insert(a);
        a->session = this;
        return a;
    }
    Actor*      createActor(const std::string& type) {
        return 0;
    }/*
    Actor* createActor(rttr::type t) {

    }*/
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
