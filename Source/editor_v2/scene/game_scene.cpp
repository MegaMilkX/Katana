#include "game_scene.hpp"

#include "../behavior/behavior.hpp"

#include "../../common/util/levenshtein_distance.hpp"

#include "../../common/util/zip_writer.hpp"
#include "../../common/util/zip_reader.hpp"

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
    return objects[i].get();
}
GameObject* GameScene::getObject(const std::string& name) {
    return getObject(name, rttr::type::get<GameObject>());
}
GameObject* GameScene::getObject(const std::string& name, rttr::type type) {
    for(auto o : objects) {
        if(o->getName() == name && o->get_type().is_derived_from(type)) {
            return o.get();
        }
    }
    return 0;
}
GameObject* GameScene::findObject(const std::string& name) {
    for(auto o : objects) {
        if(o->getName() == name) {
            return o.get();
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

Attribute* GameScene::findComponent(const std::string& object_name, rttr::type component_type) {
    Attribute* c = 0;
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
        if(objects[i].get() == o) {
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
        if(objects[i].get() == o) {
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
        _unregObject(o.get());
        getEventMgr().postObjectRemoved(o.get());
    }
    objects.clear();
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

void GameScene::refreshAabbs() {
    for(size_t i = 0; i < objectCount(); ++i) {
        getObject(i)->refreshAabb();
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

    for(auto o : objects) {
        o->getTransform()->_frameClean();
    }
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

struct GOPlain {
    std::string name;
    uint64_t parent;
    gfxm::vec3 t;
    gfxm::quat r;
    gfxm::vec3 s;
};
struct AttribPlain {
    uint64_t owner;
    std::vector<char> buf;
};
struct AttribPlainPack {
    rttr::type t;
    std::vector<AttribPlain> attribs;
};

struct ScenePlainPack {
    std::vector<GOPlain> objects;
    std::vector<AttribPlainPack> attribs;

    std::map<GameObject*, uint64_t> oid_map;
    std::map<Attribute*, uint64_t> aid_map;
};

static void first_pass(GameObject* o, ScenePlainPack& pack) {
    for(size_t i = 0; i < o->childCount(); ++i) {
        first_pass(o->getChild(i).get(), pack);
    }
    size_t id = pack.oid_map.size();
    pack.oid_map[o] = id;
}

static void second_pass(GameObject* o, ScenePlainPack& pack) {
    for(size_t i = 0; i < o->childCount(); ++i) {
        second_pass(o->getChild(i).get(), pack);
    }
    size_t id = pack.oid_map[o];
    auto& obj = pack.objects[id];
    obj.name = o->getName();
    if(o->getParent()) {
        obj.parent = pack.oid_map[o->getParent()];
    } else {
        obj.parent = -1;
    }
    obj.t = o->getTransform()->getPosition();
    obj.r = o->getTransform()->getRotation();
    obj.s = o->getTransform()->getScale();
}

void GameScene::serialize(out_stream& out) {
    ScenePlainPack scene_pack;
    for(auto o : objects) {
        first_pass(o.get(), scene_pack);
    }
    scene_pack.objects.resize(scene_pack.oid_map.size());
    for(auto o : objects) {
        second_pass(o.get(), scene_pack);
    }
    
    ZipWriter zw;
    if(!zw.isValid()) {
        LOG_WARN("GameScene::serialize: zip writer is invalid");
        return;
    }
    {
        dstream objects_strm;
        DataWriter dw(&objects_strm);
        dw.write<uint64_t>(scene_pack.objects.size());
        for(auto& o : scene_pack.objects) {
            dw.write(o.name);
            dw.write(o.parent);
            dw.write(o.t);
            dw.write(o.r);
            dw.write(o.s);
        }
        zw.add("objects", objects_strm.getBuffer());
    }

    std::vector<char> buf = zw.finalize();
    out.write(buf.data(), buf.size());
}
void GameScene::deserialize(in_stream& in) {
    std::vector<char> buf = in.read<char>(in.bytes_available());
    ZipReader zr(buf.data(), buf.size());
    std::vector<char> objects_buf = zr.extractFile("objects");
    dstream objects_strm;
    objects_strm.setBuffer(objects_buf);
    objects_buf.clear();

    DataReader dr(&objects_strm);
    uint64_t object_count = dr.read<uint64_t>();
    
    std::vector<std::shared_ptr<GameObject>> objects;
    objects.resize(object_count);
    for(uint64_t i = 0; i < object_count; ++i) {
        objects[i].reset(new GameObject());
        objects[i]->scene = this;
    }

    for(uint64_t i = 0; i < object_count; ++i) {
        objects[i]->setName(dr.readStr());
        uint64_t p = dr.read<uint64_t>();
        if(p != -1) {
            objects[i]->parent = objects[p].get();
            objects[i]->getTransform()->setParent(
                objects[p]->getTransform()
            );
            objects[p]->children.insert(objects[i]);
        } else {
            this->objects.emplace_back(
                objects[i]
            );
        }
        objects[i]->getTransform()->setPosition(dr.read<gfxm::vec3>());
        objects[i]->getTransform()->setRotation(dr.read<gfxm::quat>());
        objects[i]->getTransform()->setScale(dr.read<gfxm::vec3>());
    }

    for(uint64_t i = 0; i < object_count; ++i) {
        auto o = objects[i].get();
        _regObject(o);
        getEventMgr().postObjectCreated(o);
    }
}

bool GameScene::serialize(const std::string& fname) {
    file_stream strm(fname, file_stream::F_OUT);
    if(!strm.is_open()) {
        return false;
    }

    serialize(strm);

    return true;
}
bool GameScene::deserialize(const std::string& fname) {
    file_stream strm(fname, file_stream::F_IN);
    if(!strm.is_open()) {
        return false;
    }

    deserialize(strm);

    return true;
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