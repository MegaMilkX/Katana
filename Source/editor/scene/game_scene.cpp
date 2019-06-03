#include "game_scene.hpp"

#include "../behavior/behavior.hpp"

#include "../../common/util/levenshtein_distance.hpp"

#include "../../common/util/zip_writer.hpp"
#include "../../common/util/zip_reader.hpp"

GameScene::GameScene() {
    root_object.reset(new GameObject());
    root_object->scene = this;
}

GameScene::~GameScene() {
}

void GameScene::clear() {
    root_object.reset(new GameObject());
    root_object->scene = this;

    controllers.clear();
    updatable_controllers.clear();
}

GameObject* GameScene::getRoot() {
    return root_object.get();
}

std::vector<GameObject*> GameScene::findObjectsFuzzy(const std::string& name) {
    std::vector<GameObject*> r;
    root_object->getAllObjects(r);

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

std::vector<Attribute*>& GameScene::getAllComponents(rttr::type t) {
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

void GameScene::createBehavior(GameObject* owner, rttr::type t) {
    if(!t.is_valid()) {
        LOG_WARN(t.get_name().to_string() << " - not a valid Behavior type");
        return;
    }
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr construction policy must be pointer");
        return;
    }
    Behavior* b = v.get_value<Behavior*>();
    if(!b) {
        LOG_WARN(
            "Failed to create behavior of type " <<
            t.get_name().to_string()
        );
        return;
    }
    b->object = owner;

    behaviors[owner].reset(b);
    b->onInit();
}
Behavior* GameScene::findBehavior(GameObject* owner) {
    auto it = behaviors.find(owner);
    if(it == behaviors.end()) {
        return 0;
    }
    return it->second.get();
}
void GameScene::eraseBehavior(GameObject* owner) {
    auto b = findBehavior(owner);
    if(b) {
        b->onCleanup();
        behaviors.erase(owner);
    }
}

void GameScene::startSession() {
    for(auto& kv : controllers) {
        kv.second->onStart();
    }

    for(auto& kv : behaviors) {
        kv.second->onStart();
    }
}
void GameScene::stopSession() {    
    for(auto& kv : controllers) {
        kv.second->onEnd();
    }
}

void GameScene::update() {
    for(auto& kv : behaviors) {
        kv.second->onUpdate();
    }
    
    for(auto c : updatable_controllers) {
        c->onUpdate();
    }

    root_object->getTransform()->_frameClean();
}
void GameScene::debugDraw(DebugDraw& dd) {
    for(auto& kv : controllers) {
        kv.second->debugDraw(dd);
    }
}

void GameScene::resetAttribute(Attribute* attrib) {
    _unregisterComponent(attrib);
    _registerComponent(attrib);   
}
void GameScene::_registerComponent(Attribute* c) {
    object_components[c->get_type()].emplace_back(c);

    for(auto& kv : controllers) {
        kv.second->attribCreated(c->get_type(), c);
    }
    auto bases = c->get_type().get_base_classes();
    for(auto b : bases) {
        for(auto& kv : controllers) {
            kv.second->attribCreated(b, c);
        }   
    }
}
void GameScene::_unregisterComponent(Attribute* c) {
    auto& vec = object_components[c->get_type()];
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i] == c) {
            vec.erase(vec.begin() + i);
            break;
        }
    }
    
    for(auto& kv : controllers) {
        kv.second->attribDeleted(c->get_type(), c);
    }
    auto bases = c->get_type().get_base_classes();
    for(auto b : bases) {
        for(auto& kv : controllers) {
            kv.second->attribDeleted(b, c);
        }   
    }
}
void GameScene::_regObject(GameObject* o) {
    // TODO: Trigger callbacks
}
void GameScene::_unregObject(GameObject* o) {
    // TODO: Trigger callbacks
}

void GameScene::write(out_stream& out) {
    
}
void GameScene::read(in_stream& in) {
    
}

bool GameScene::write(const std::string& fname) {
    file_stream strm(fname, file_stream::F_OUT);
    if(!strm.is_open()) {
        return false;
    }

    write(strm);

    return true;
}
bool GameScene::read(const std::string& fname) {
    file_stream strm(fname, file_stream::F_IN);
    if(!strm.is_open()) {
        return false;
    }

    read(strm);

    return true;
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