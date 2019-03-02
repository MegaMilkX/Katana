#ifndef SCENE_EVENT_MGR_HPP
#define SCENE_EVENT_MGR_HPP

#include <functional>
#include <map>
#include <list>
#include <set>
#include "scene_object.hpp"
#include "scene_listener.hpp"

class SceneEventBroadcaster {
public:
    void post(GameObject* sender, SCENE_EVENT evt, rttr::variant payload = rttr::variant()) {
        auto it = listeners.find(evt);
        if(it != listeners.end()) {
            auto& list = it->second;
            for(auto lit = list.begin(); lit != list.end(); ++lit) {
                (*lit)->onSceneEvent(sender, evt, payload);
            }
        }
    }

    template<typename T>
    void post(GameObject* sender, SCENE_EVENT e, const T& payload) {
        post(sender, e, rttr::variant(payload));
    }

    void subscribe(SceneListener* listener, SCENE_EVENT evt) {
        listeners[evt].insert(listener);
    }
    void unsubscribe(SceneListener* listener, SCENE_EVENT evt) {
        listeners[evt].erase(listener);
    }
    void unsubscribeAll(SceneListener* listener) {
        for(auto& kv : listeners) {
            kv.second.erase(listener);
        }
    }
private:
    std::map<
        SCENE_EVENT, 
        std::set<
            SceneListener*
        >
    > listeners;
};

#endif
