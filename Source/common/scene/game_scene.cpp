#include <stack>

#include "game_scene.hpp"

#include "../../common/util/levenshtein_distance.hpp"

#include "../../common/util/zip_writer.hpp"
#include "../../common/util/zip_reader.hpp"

#include "../util/has_suffix.hpp"

#include "../util/scene_object_from_fbx.hpp"

GameScene::GameScene() {
    setName("Root");
}

GameScene::~GameScene() {
}

void GameScene::clear() {
    deleteAllComponents();
    for(auto o : children) {
        delete o;
    }
    children.clear();

    for(auto& kv : controllers) {
        kv.second->cleanup();
    }
    //controllers.clear();
    //updatable_controllers.clear();
}

std::vector<ktNode*> GameScene::findObjectsFuzzy(const std::string& name) {
    std::vector<ktNode*> r;
    getAllObjects(r);

    std::vector<ktNode*> result;
    struct tmp {
        ktNode* o;
        size_t cost;
    };
    std::vector<tmp> sorted;
    for(auto o : r) {
        if(name.size() > o->getName().size()) {
            continue;
        }
        std::string str_to_compare = o->getName();
        size_t find_from = str_to_compare.find_first_of(name[0]);
        if(find_from == str_to_compare.npos) continue;
        str_to_compare = str_to_compare.substr(find_from);
        if(name.size() < str_to_compare.size()) {
            str_to_compare = str_to_compare.substr(0, name.size());
        }
        size_t cost = levenshteinDistance(name, str_to_compare) * 10;
        if(find_from > 0) cost += 1;
        cost += o->getName().size() - name.size();
        size_t pos = o->getName().find_first_of(name);
        if(pos != o->getName().npos) {
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

void GameScene::addListener(SceneController* lis) {
    foreign_listeners.insert(lis);
}
void GameScene::removeListener(SceneController* lis) {
    foreign_listeners.erase(lis);
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

    getTransform()->_frameClean();
}
void GameScene::debugDraw(DebugDraw& dd) {
    for(auto& kv : controllers) {
        kv.second->debugDraw(dd);
    }
}

void GameScene::serialize(out_stream& out) {
    write(out);
}
bool GameScene::deserialize(in_stream& in, size_t sz) {
    read(in);
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

    // Trigger component signals only for the new controller (don't want to needlessly disturb other controllers)
    for(auto& kv : object_components) {
        for(auto& a : kv.second) {
            sc->attribCreated(a->get_type(), a);
            auto bases = a->get_type().get_base_classes();
            for(auto b : bases) {
                sc->attribCreated(b, a);
            }
        }
    }
}


void GameScene::_registerComponent(Attribute* c) {
    bool attrib_found = false;
    auto& vec = object_components[c->get_type()];
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i] == c) {
            attrib_found = true;
            break;
        }
    }

    if(attrib_found) {
        return;
    }

    vec.emplace_back(c);

    for(auto& kv : controllers) {
        kv.second->attribCreated(c->get_type(), c);
    }
    auto bases = c->get_type().get_base_classes();
    for(auto b : bases) {
        for(auto& kv : controllers) {
            kv.second->attribCreated(b, c);
        }   
    }

    for(auto l : foreign_listeners) {
        l->attribCreated(c->get_type(), c);
        for(auto b : bases) {
            l->attribCreated(b, c);
        }
    }
}
void GameScene::_unregisterComponent(Attribute* c) {
    bool attrib_found = false;
    auto& vec = object_components[c->get_type()];
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i] == c) {
            vec.erase(vec.begin() + i);
            attrib_found = true;
            break;
        }
    }

    if(!attrib_found) {
        return;
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

    for(auto l : foreign_listeners) {
        l->attribDeleted(c->get_type(), c);
        for(auto b : bases) {
            l->attribDeleted(b, c);
        }
    }
}
void GameScene::_readdComponent(Attribute* attrib) {
    _unregisterComponent(attrib);
    _registerComponent(attrib);   
}
void GameScene::_signalAttribTransform(Attribute* c) {
    for(auto& kv : controllers) {
        kv.second->attribTransformed(c->get_type(), c);
    }
    auto bases = c->get_type().get_base_classes();
    for(auto b : bases) {
        for(auto& kv : controllers) {
            kv.second->attribTransformed(b, c);
        }   
    }

    for(auto l : foreign_listeners) {
        l->attribTransformed(c->get_type(), c);
        for(auto b : bases) {
            l->attribTransformed(b, c);
        }
    }
}