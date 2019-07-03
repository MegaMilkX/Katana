#ifndef SCENE_EVENT_MGR_HPP
#define SCENE_EVENT_MGR_HPP

#include <functional>
#include <map>
#include <list>
#include <set>
#include "game_object.hpp"
#include "scene_listener.hpp"

class SceneEventBroadcaster {
public:
    void postObjectCreated(GameObject* o) {
        auto it = listeners.find(EVT_OBJECT_CREATED);
        if(it != listeners.end()) {
            auto& list = it->second;
            for(auto lit = list.begin(); lit != list.end(); ++lit) {
                (*lit)->onObjectCreated(o);
            }
        } 
    }
    void postObjectRemoved(GameObject* o) {
        auto it = listeners.find(EVT_OBJECT_REMOVED);
        if(it != listeners.end()) {
            auto& list = it->second;
            for(auto lit = list.begin(); lit != list.end(); ++lit) {
                (*lit)->onObjectRemoved(o);
            }
        } 
    }
    void postComponentCreated(Attribute* o) {
        auto it = listeners.find(EVT_COMPONENT_CREATED);
        if(it != listeners.end()) {
            auto& list = it->second;
            for(auto lit = list.begin(); lit != list.end(); ++lit) {
                (*lit)->onComponentCreated(o);
            }
        } 
    }
    void postComponentRemoved(Attribute* o) {
        auto it = listeners.find(EVT_COMPONENT_REMOVED);
        if(it != listeners.end()) {
            auto& list = it->second;
            for(auto lit = list.begin(); lit != list.end(); ++lit) {
                (*lit)->onComponentRemoved(o);
            }
        } 
    }

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
