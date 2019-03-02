#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "scene_object.hpp"
#include "scene_event_mgr.hpp"

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

class GameScene {
public:
    ~GameScene();

    SceneEventBroadcaster& getEventMgr();

    void copy(GameScene* other);

    size_t objectCount() const;
    GameObject* getObject(size_t i);
    GameObject* findObject(const std::string& name);

    template<typename T>
    T* create();
    GameObject* create(rttr::type t);
    GameObject* copyObject(GameObject* o);
    void remove(GameObject* o);
    void removeAll();
private:
    std::vector<GameObject*> objects;
    
    SceneEventBroadcaster event_broadcaster;
};

template<typename T>
T* GameScene::create() {
    T* o = new T();
    o->scene = this;
    objects.emplace_back(o);
    getEventMgr().post(o, EVT_OBJECT_CREATED);
    return o;
}

#endif
