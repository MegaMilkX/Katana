#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "game_object.hpp"
#include "scene_event_mgr.hpp"

#include "../components/camera.hpp"

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

class GameScene : public SceneListener {
public:
    GameScene();
    ~GameScene();

    SceneEventBroadcaster& getEventMgr();

    void clear();

    void copy(GameScene* other);

    size_t objectCount() const;
    GameObject* getObject(size_t i);
    GameObject* getObject(const std::string& name);
    GameObject* getObject(const std::string& name, rttr::type type);
    GameObject* findObject(const std::string& name);
    GameObject* findObject(const std::string& name, rttr::type type);
    template<typename T>
    T*          findObject(const std::string& name);

    template<typename T>
    T* create();
    GameObject* create(rttr::type t);
    GameObject* copyObject(GameObject* o);
    void remove(GameObject* o);
    void removeRecursive(GameObject* o);
    void removeAll();

    void refreshAabbs();

    void setDefaultCamera(CmCamera* c);
    CmCamera* getDefaultCamera();

    void resetActors();
    void update();
private:
    void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload);

    GameObject* createEmptyCopy(GameObject* o);
    GameObject* copyToExistingObject(GameObject* o);

    std::vector<GameObject*> objects;
    CmCamera* default_camera = 0;
    std::set<ActorObject*> actors;
    
    SceneEventBroadcaster event_broadcaster;
};

template<typename T>
T* GameScene::create() {
    T* o = new T();
    o->scene = this;
    objects.emplace_back(o);
    notifyObjectEvent(this, o, EVT_OBJECT_CREATED, o->get_type());
    return o;
}

template<typename T>
T* GameScene::findObject(const std::string& name) {
    return (T*)findObject(name, rttr::type::get<T>());
}

#endif
