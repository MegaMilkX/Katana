#include "object_instance.hpp"

#include "game_scene.hpp"

void ktObjectInstance::setScene(std::shared_ptr<GameScene> scene) {
    if(this->scene) {
        this->scene->_unlinkInstance(this);
    }
    this->scene = scene;
    this->scene->_linkInstance(this);

    // TODO:
    copy(scene.get(), OBJECT_FLAG_TRANSIENT);
}
GameScene* ktObjectInstance::getScene() const {
    return scene.get();
}

void ktObjectInstance::_createInstancedNode(GameObject* o) {
    std::vector<GameObject*> path;
    GameObject* current = o;
    while(current) {
        if(current->getParent()) {
            path.emplace_back(current);
            current = current->getParent();
        } else {
            current = 0;
        }
    }

    current = this;
    for(int i = path.size() - 1; i >= 0; --i) {
        GameObject* ch =  current->getChild(path[i]->getName());
        if(!ch) {
            auto o = current->createChild(OBJECT_FLAG_TRANSIENT);
            o->getTransform()->instantiate(path[i]->getTransform());
            o->copy(path[i]);
            original_to_instance.insert(std::make_pair(path[i], o));
            break;
        } else {
            current = ch;
        }
    }
}
void ktObjectInstance::_removeInstancedNode(GameObject* o) {
    auto it = original_to_instance.find(o);
    if(it == original_to_instance.end()) {
        LOG_WARN("Couldn't find instanced child node for '" << o->getName() << "'");
        return;
    }
    it->second->remove();
    original_to_instance.erase(it);
}

void ktObjectInstance::_createInstancedAttrib(Attribute* a) {
    std::vector<GameObject*> path;
    GameObject* current = a->getOwner();
    while(current) {
        if(current->getParent()) {
            path.emplace_back(current);
            current = current->getParent();
        } else {
            current = 0;
        }
    }

    current = this;
    for(int i = path.size() - 1; i >= 0; --i) {
        if(!current) break;
        GameObject* ch =  current->getChild(path[i]->getName());
        current = ch;
    }

    if(!current) {
        LOG_WARN("Failed to find instanced child node to clone attribute.");
        return;
    } 

    auto new_attrib = current->get(a->get_type());
    auto parent_cache = getParent();
    parent = 0;
    new_attrib->instantiate(*a);
    parent = parent_cache;
}
void ktObjectInstance::_removeInstancedAttrib(Attribute* a) {
    auto it = original_to_instance.find(a->getOwner());
    if(it == original_to_instance.end()) {
        LOG_WARN("Couldn't find instanced child node for '" << a->getOwner()->getName() << "', while attempting to delete instanced attribute");
        return;
    }
    // TODO: Remove attribute
    // it->second->removeAttrib(a->get_type()) or smth
}