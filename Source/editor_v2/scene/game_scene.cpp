#include "game_scene.hpp"

#include "../../common/util/levenshtein_distance.hpp"

void notifyObjectEvent(GameScene* scn, GameObject* o, SCENE_EVENT evt, rttr::type t) {
    scn->getEventMgr().post(
        o,
        evt,
        t
    );
}

GameScene::GameScene() {
}

GameScene::~GameScene() {
    getEventMgr().post(0, EVT_SCENE_DESTROYED);
    removeAll();
}

SceneEventBroadcaster& GameScene::getEventMgr() {
    return event_broadcaster;
}

void GameScene::clear() {
    removeAll();
    controllers.clear();
    updatable_controllers.clear();
}

void GameScene::copy(GameScene* other) {
    for(size_t i = 0; i < other->objectCount(); ++i) {
        createEmptyCopy(other->getObject(i));
    }
    for(size_t i = 0; i < other->objectCount(); ++i) {
        copyToExistingObject(other->getObject(i));
    }
    for(size_t i = 0; i < other->controllerCount(); ++i) {
        getController(other->getController(i)->get_type())->copy(*other->getController(i));
    }
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
std::vector<GameObject*> GameScene::findObjectsFuzzy(const std::string& name) {
    std::vector<GameObject*> r;
    for(auto o : objects) {
        o->getAllObjects(r);
    }
    std::vector<GameObject*> result;
    struct tmp {
        GameObject* o;
        size_t cost;
    };
    std::vector<tmp> sorted;
    for(auto o : r) {
        size_t cost = levenshteinDistance(name, o->getName());
        size_t pos = o->name.find_first_of(name);
        if(pos != o->name.npos) {
            //cost = 0;
        }
        sorted.emplace_back(tmp{o, cost});
    }
    std::sort(sorted.begin(), sorted.end(), [](const tmp& a, const tmp& b)->bool{
        return a.cost < b.cost;
    });
    for(size_t i = 0; i < sorted.size() && i < 10; ++i) {

        result.emplace_back(sorted[i].o);
    }
    return result;
}

ObjectComponent* GameScene::findComponent(const std::string& object_name, rttr::type component_type) {
    ObjectComponent* c = 0;
    auto o = findObject(object_name);
    if(o) {
        c = o->find(component_type).get();
        if(!c) {
            LOG_WARN("Object '" << object_name << "' has no component '" << component_type.get_name().to_string() << "'");
        }
    } else {
        LOG_WARN("Failed to find object '" << object_name << "'");
    }
    return c;
}

std::vector<GameObject*>& GameScene::getObjects(rttr::type t) {
    return typed_objects[t];
}

std::vector<ObjectComponent*>& GameScene::getAllComponents(rttr::type t) {
    return object_components[t];
}

SceneController* GameScene::getController(rttr::type t) {
    if(!hasController(t)) {
        createController(t);
    }
    return controllers[t].get();
}
bool GameScene::hasController(rttr::type t) {
    return controllers.count(t) != 0;
}
size_t GameScene::controllerCount() const {
    return controllers.size();
}
SceneController* GameScene::getController(size_t i) {
    auto it = controllers.begin();
    std::advance(it, i);
    return it->second.get();
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
    o->_onCreate();
    _regObject(o);
    getEventMgr().postObjectCreated(o);
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
            _unregObject(o);
            getEventMgr().postObjectRemoved(o);
            delete o;
            objects.erase(objects.begin() + i);
            break;
        }
    }
}
void GameScene::removeAll() {
    for(auto o : objects) {
        _unregObject(o);
        getEventMgr().postObjectRemoved(o);
        delete o;
    }
    objects.clear();
}

void GameScene::refreshAabbs() {
    for(size_t i = 0; i < objectCount(); ++i) {
        getObject(i)->refreshAabb();
    }
}

void GameScene::startSession() {
    for(auto& kv : controllers) {
        kv.second->onStart();
    }
}
void GameScene::stopSession() {
    for(auto& kv : controllers) {
        kv.second->onEnd();
    }
}

void GameScene::update() {
    for(auto c : updatable_controllers) {
        c->onUpdate();
    }

    for(auto o : objects) {
        o->getTransform()->_frameClean();
    }
}
void GameScene::debugDraw(DebugDraw& dd) {
    for(auto& kv : controllers) {
        kv.second->debugDraw(dd);
    }
}

void GameScene::_registerComponent(ObjectComponent* c) {
    object_components[c->get_type()].emplace_back(c);
}
void GameScene::_unregisterComponent(ObjectComponent* c) {
    auto& vec = object_components[c->get_type()];
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i] == c) {
            vec.erase(vec.begin() + i);
            break;
        }
    }
}
void GameScene::_regObject(GameObject* o) {
    auto t = o->get_type();
    typed_objects[o->get_type()].emplace_back(o);
    auto derived = t.get_base_classes();
    for(auto tt : derived) {
        typed_objects[tt].emplace_back(o);
    }
}
void GameScene::_unregObject(GameObject* o) {
    auto t = o->get_type();
    auto& vec = typed_objects[t];
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i] == o) {
            vec.erase(vec.begin() + i);
            break;
        }
    }

    auto derived = t.get_base_classes();
    for(auto tt : derived) {
        auto& vec = typed_objects[tt];
        for(size_t i = 0; i < vec.size(); ++i) {
            if(vec[i] == o) {
                vec.erase(vec.begin() + i);
                break;
            }
        }
    }
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

SceneController* GameScene::createController(rttr::type t) {
    if(!t.is_valid()) {
        LOG_WARN("Scene controller type is not valid: " << t.get_name().to_string());
        return 0;
    }
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
        return 0;
    }
    SceneController* sc = v.get_value<SceneController*>();
    if(!sc) {
        LOG_WARN("Failed to create scene controller: " << t.get_name().to_string());
        return 0;
    }
    controllers[t].reset(sc);

    if(sc->getInfo().auto_update) {
        updatable_controllers.emplace_back(sc);
        std::sort(
            updatable_controllers.begin(),
            updatable_controllers.end(),
            [](const SceneController* a, const SceneController* b)->bool{
                return a->getInfo().priority < b->getInfo().priority;
            }
        );
    }

    sc->init(this);
}