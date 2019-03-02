#include "scene_object.hpp"

#include "game_scene.hpp"

GameObject::GameObject()
: uid((uint64_t)this) {

}

void GameObject::setUid(uint64_t uid) {
    this->uid = uid;
}
uint64_t GameObject::getUid() const {
    return uid;
}

void GameObject::setName(const std::string& name) { 
    this->name = name; 
}
const std::string& GameObject::getName() const { 
    return name; 
}

GameScene* GameObject::getScene() {
    return scene;
}
GameObject* GameObject::getRoot() {
    if(!parent) {
        return this;
    } else {
        return parent->getRoot();
    }
}

TransformNode* GameObject::getTransform() {
    return &transform;
}

void GameObject::copy(GameObject* other) {
    setName(other->getName());
    getTransform()->setPosition(other->getTransform()->getPosition());
    getTransform()->setRotation(other->getTransform()->getRotation());
    getTransform()->setScale(other->getTransform()->getScale());
}
void GameObject::copyRecursive(GameObject* o) {
    copy(o);
    for(size_t i = 0; i < o->childCount(); ++i) {
        auto n = createChild(o->getChild(i)->get_type());
        n->copyRecursive(o->getChild(i).get());
    }
}
void GameObject::copyComponents(GameObject* other) {
    for(size_t i = 0; i < other->componentCount(); ++i) {
        auto c = other->getById(i);
        auto new_c = createComponent(c->get_type());
        new_c->copy(c.get());
    }
}
void GameObject::copyComponentsRecursive(GameObject* other) {
    for(size_t i = 0; i < other->childCount(); ++i) {
        auto child = getChild(other->getChild(i)->getName());
        if(!child) continue;
        child->copyComponentsRecursive(other->getChild(i).get());
    }
    copyComponents(other);
}

std::shared_ptr<GameObject> GameObject::createChild() {
    return createChild(rttr::type::get<GameObject>());
}
std::shared_ptr<GameObject> GameObject::createChild(rttr::type t) {
    if(!t.is_valid()) {
        LOG_WARN(t.get_name().to_string() << " - not a valid type");
        return std::shared_ptr<GameObject>();
    }
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
        return std::shared_ptr<GameObject>();
    }
    GameObject* c = v.get_value<GameObject*>();
    if(!c) {
        LOG_WARN("Failed to create component " << t.get_name().to_string());
        return std::shared_ptr<GameObject>();
    }
    auto o = std::shared_ptr<GameObject>(c);
    o->scene = scene;
    o->parent = this;
    o->getTransform()->setParent(&transform);
    children.insert(o);
    scene->getEventMgr().post(o.get(), EVT_OBJECT_CREATED);
    return o;
}
size_t GameObject::childCount() {
    return children.size();
}
std::shared_ptr<GameObject> GameObject::getChild(size_t i) {
    auto it = children.begin();
    std::advance(it, i);
    return (*it);
}
std::shared_ptr<GameObject> GameObject::getChild(const std::string& name) {
    for(auto o : children) {
        if(o->getName() == name) {
            return o;
        }
    }
    return std::shared_ptr<GameObject>();
}
GameObject* GameObject::findObject(const std::string& name) {
    for(auto o : children) {
        if(o->getName() == name) return o.get();
        GameObject* r = o->findObject(name);
        if(r) return r;
    }
    return 0;
}

std::shared_ptr<ObjectComponent> GameObject::find(rttr::type component_type) {
    if(components.count(component_type) == 0) {
        return std::shared_ptr<ObjectComponent>();
    }
    return components[component_type];
}
std::shared_ptr<ObjectComponent> GameObject::get(rttr::type component_type) {
    auto c = find(component_type);
    if(!c) {
        return createComponent(component_type);
    }
    return c;
}
size_t GameObject::componentCount() {
    return components.size();
}
std::shared_ptr<ObjectComponent> GameObject::getById(size_t id) {
    auto it = components.begin();
    std::advance(it, id);
    return it->second;
}

IEditorObjectDesc* GameObject::_newEditorObjectDesc() {
    return new EditorGameObjectDesc(this);
}

// ==== Private ====================================

std::shared_ptr<ObjectComponent> GameObject::createComponent(rttr::type t) {
    if(!t.is_valid()) {
        LOG_WARN(t.get_name().to_string() << " - not a valid type");
        return std::shared_ptr<ObjectComponent>();
    }
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
        return std::shared_ptr<ObjectComponent>();
    }
    ObjectComponent* c = v.get_value<ObjectComponent*>();
    if(!c) {
        LOG_WARN("Failed to create component " << t.get_name().to_string());
        return std::shared_ptr<ObjectComponent>();
    }
    c->owner = this;
    auto ptr = std::shared_ptr<ObjectComponent>(c);
    components[t] = ptr;
    scene->getEventMgr().post(this, EVT_COMPONENT_CREATED, t);
    return ptr;
}