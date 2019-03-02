#include "game_scene.hpp"

GameScene::~GameScene() {
    getEventMgr().post(0, EVT_SCENE_DESTROYED);
    removeAll();
}

SceneEventBroadcaster& GameScene::getEventMgr() {
    return event_broadcaster;
}

void GameScene::copy(GameScene* other) {
    for(size_t i = 0; i < other->objectCount(); ++i) {
        copyObject(other->getObject(i));
    }
}

size_t GameScene::objectCount() const {
    return objects.size();
}
GameObject* GameScene::getObject(size_t i) {
    return objects[i];
}
GameObject* GameScene::findObject(const std::string& name) {
    for(auto o : objects) {
        GameObject* r = 0;
        if(r = o->findObject(name)) {
            return r;
        }
    }
    return 0;
}

GameObject* GameScene::create(rttr::type t) {
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
        return 0;
    }
    GameObject* o = v.get_value<GameObject*>();
    o->scene = this;
    objects.emplace_back(o);
    getEventMgr().post(o, EVT_OBJECT_CREATED);
    return o;
}
GameObject* GameScene::copyObject(GameObject* o) {
    auto n = create(o->get_type());
    n->copyRecursive(o);
    n->copyComponentsRecursive(o);
    return n;
}
void GameScene::remove(GameObject* o) {
    for(size_t i = 0; i < objects.size(); ++i) {
        if(objects[i] == o) {
            getEventMgr().post(o, EVT_OBJECT_REMOVED);
            delete o;
            objects.erase(objects.begin() + i);
            break;
        }
    }
}
void GameScene::removeAll() {
    for(auto o : objects) {
        getEventMgr().post(o, EVT_OBJECT_REMOVED);
        delete o;
    }
    objects.clear();
}