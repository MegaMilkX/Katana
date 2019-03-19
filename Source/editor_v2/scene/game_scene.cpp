#include "game_scene.hpp"

void notifyObjectEvent(GameScene* scn, GameObject* o, SCENE_EVENT evt, rttr::type t) {
    scn->getEventMgr().post(
        o,
        evt,
        t
    );/*
    auto base = t.get_base_classes();
    for(auto& b : base) {
        scn->getEventMgr().post(o, evt, b);
    }*/
}

GameScene::GameScene() {
    getEventMgr().subscribe(this, EVT_OBJECT_CREATED);
    getEventMgr().subscribe(this, EVT_OBJECT_REMOVED);
    getEventMgr().subscribe(this, EVT_COMPONENT_REMOVED);
}

GameScene::~GameScene() {
    getEventMgr().unsubscribeAll(this);
    getEventMgr().post(0, EVT_SCENE_DESTROYED);
    removeAll();
}

SceneEventBroadcaster& GameScene::getEventMgr() {
    return event_broadcaster;
}

void GameScene::clear() {
    default_camera = 0;
    removeAll();
}

void GameScene::copy(GameScene* other) {
    for(size_t i = 0; i < other->objectCount(); ++i) {
        createEmptyCopy(other->getObject(i));
    }
    for(size_t i = 0; i < other->objectCount(); ++i) {
        copyToExistingObject(other->getObject(i));
    }
    if(other->default_camera) {
        auto o = findObject(other->default_camera->getOwner()->getName());
        if(o) 
            default_camera = o->find<CmCamera>().get();
    }

    resetActors();
}

size_t GameScene::objectCount() const {
    return objects.size();
}
GameObject* GameScene::getObject(size_t i) {
    return objects[i];
}
GameObject* GameScene::getObject(const std::string& name) {
    return getObject(name, rttr::type::get<GameObject>());
}
GameObject* GameScene::getObject(const std::string& name, rttr::type type) {
    for(auto o : objects) {
        if(o->getName() == name && o->get_type().is_derived_from(type)) {
            return o;
        }
    }
    return 0;
}
GameObject* GameScene::findObject(const std::string& name) {
    for(auto o : objects) {
        if(o->getName() == name) {
            return o;
        }
        GameObject* r = 0;
        if(r = o->findObject(name)) {
            return r;
        }
    }
    return 0;
}
GameObject* GameScene::findObject(const std::string& name, rttr::type type) {
    auto o = findObject(name);
    if(o && o->get_type().is_derived_from(type)) {
        return o;
    } else {
        return 0;
    }
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
    notifyObjectEvent(this, o, EVT_OBJECT_CREATED, t);
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
            for(size_t i = 0; i < o->childCount(); ++i) {
                auto n = create(o->getChild(i)->get_type());
                n->copyRecursive(o->getChild(i).get());
                n->copyComponentsRecursive(o->getChild(i).get());
            }
        }
    }
    removeRecursive(o);
}
void GameScene::removeRecursive(GameObject* o) {
    for(size_t i = 0; i < objects.size(); ++i) {
        if(objects[i] == o) {
            notifyObjectEvent(this, o, EVT_OBJECT_REMOVED, o->get_type());
            delete o;
            objects.erase(objects.begin() + i);
            break;
        }
    }
}
void GameScene::removeAll() {
    for(auto o : objects) {
        notifyObjectEvent(this, o, EVT_OBJECT_REMOVED, o->get_type());
        delete o;
    }
    objects.clear();
}

void GameScene::refreshAabbs() {
    for(size_t i = 0; i < objectCount(); ++i) {
        getObject(i)->refreshAabb();
    }
}

void GameScene::setDefaultCamera(CmCamera* c) {
    default_camera = c;
}
CmCamera* GameScene::getDefaultCamera() {
    return default_camera;
}

void GameScene::resetActors() {
    for(auto a : actors) {
        a->reset();
    }
}
void GameScene::update() {
    for(auto a : actors) {
        a->update();
    }
}

void GameScene::onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) {
    switch(e) {
    case EVT_OBJECT_CREATED:
        if(payload.get_value<rttr::type>().is_derived_from<ActorObject>()) {
            ((ActorObject*)sender)->init();
            actors.insert((ActorObject*)sender);
        }
        break;
    case EVT_OBJECT_REMOVED:
        if(payload.get_value<rttr::type>().is_derived_from<ActorObject>()) {
            actors.erase((ActorObject*)sender);
        }
        break;
    case EVT_COMPONENT_CREATED:
        break;
    case EVT_COMPONENT_REMOVED:
        if(payload.get_value<rttr::type>() == rttr::type::get<CmCamera>()) {
            if(sender->get<CmCamera>().get() == default_camera) {
                default_camera = 0;
            }
        }
        break;
    };
}

GameObject* GameScene::createEmptyCopy(GameObject* o) {
    auto n = create(o->get_type());
    n->copyEmptyTree(o);
    return n;
}
GameObject* GameScene::copyToExistingObject(GameObject* o) {
    auto n = getObject(o->getName(), o->get_type());
    if(n) {
        n->cloneExistingTree(o);
        n->copyComponentsRecursive(o);
    }
    return n;
}