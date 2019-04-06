#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "game_object.hpp"
#include "scene_event_mgr.hpp"

#include "../components/camera.hpp"

#include "../../common/debug_draw.hpp"

class SpatialObject : public GameObject {
    RTTR_ENABLE(GameObject)
public:

};
STATIC_RUN(SpatialObject) {
    rttr::registration::class_<SpatialObject>("SpatialObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

class StaticMesh : public SpatialObject {
    RTTR_ENABLE(SpatialObject)
public:
private:
};
STATIC_RUN(StaticMesh) {
    rttr::registration::class_<StaticMesh>("StaticMesh")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

void notifyObjectEvent(GameScene* scn, GameObject* o, SCENE_EVENT evt, rttr::type t);

#include "actor_object.hpp"

#include "scene_controller.hpp"

class GameScene {
public:
    GameScene();
    ~GameScene();

    SceneEventBroadcaster& getEventMgr();

    void clear();

    void copy(GameScene* other);

    // Objects
    size_t objectCount() const;
    GameObject* getObject(size_t i);
    GameObject* getObject(const std::string& name);
    GameObject* getObject(const std::string& name, rttr::type type);
    GameObject* findObject(const std::string& name);
    GameObject* findObject(const std::string& name, rttr::type type);
    template<typename T>
    T*          findObject(const std::string& name);
    std::vector<GameObject*> findObjectsFuzzy(const std::string& name);

    ObjectComponent* findComponent(const std::string& object_name, rttr::type component_type);
    template<typename T>
    T*               findComponent(const std::string& object_name);

    template<typename T>
    T* create();
    GameObject* create(rttr::type t);
    GameObject* copyObject(GameObject* o);
    void remove(GameObject* o);
    void removeRecursive(GameObject* o);
    void removeAll();

    void refreshAabbs();

    template<typename T>
    std::vector<GameObject*>& getObjects() { return getObjects(rttr::type::get<T>()); }
    std::vector<GameObject*>& getObjects(rttr::type t);

    // Components
    template<typename T>
    std::vector<ObjectComponent*>& getAllComponents() { return getAllComponents(rttr::type::get<T>()); }
    std::vector<ObjectComponent*>& getAllComponents(rttr::type t);

    // Controllers
    template<typename T>
    T* getController();
    SceneController* getController(rttr::type t);
    template<typename T>
    bool hasController();
    bool hasController(rttr::type t);
    size_t controllerCount() const;
    SceneController* getController(size_t i);

    void startSession();
    void stopSession();

    void update();
    void debugDraw(DebugDraw& dd);

    void _registerComponent(ObjectComponent* c);
    void _unregisterComponent(ObjectComponent* c);
    void _regObject(GameObject* o);
    void _unregObject(GameObject* o);
private:
    GameObject* createEmptyCopy(GameObject* o);
    GameObject* copyToExistingObject(GameObject* o);

    SceneController* createController(rttr::type t);

    std::vector<GameObject*> objects;
    std::map<
        rttr::type, 
        std::vector<ObjectComponent*>
    > object_components;
    std::map<
        rttr::type,
        std::vector<GameObject*>
    > typed_objects;

    std::map<rttr::type, std::shared_ptr<SceneController>> controllers;
    std::vector<SceneController*> updatable_controllers;
    
    SceneEventBroadcaster event_broadcaster;
};

template<typename T>
T* GameScene::create() {
    T* o = new T();
    o->scene = this;
    objects.emplace_back(o);
    o->_onCreate();
    _regObject(o);
    getEventMgr().postObjectCreated(o);
    return o;
}

template<typename T>
T* GameScene::findObject(const std::string& name) {
    return (T*)findObject(name, rttr::type::get<T>());
}

template<typename T>
T* GameScene::findComponent(const std::string& object_name) {
    return (T*)findComponent(object_name, rttr::type::get<T>());
}

template<typename T>
T* GameScene::getController() {
    return (T*)getController(rttr::type::get<T>());
}
template<typename T>
bool GameScene::hasController() {
    return hasController(rttr::type::get<T>());
}

#endif
