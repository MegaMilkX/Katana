#include "scene_object.hpp"

#include <map>

template<typename T_KEY, typename T_VALUE>
struct Factory {
    void erase(const T_KEY k) {
        data.erase(k);
    }
    void set(const T_KEY k, T_VALUE* val) {
        data[k].reset(val);
    }
    T_VALUE* get(const T_KEY k) {
        auto it = data.find(k);
        if(it == data.end()) return 0;
        return it->second.get();
    }
private:
    std::map<T_KEY, std::shared_ptr<T_VALUE>> data;
};

struct AttribFactory {
    template<typename T>
    void        erase(const SceneObject* key) {
        erase(key, rttr::type::get<T>());
    }
    template<typename T>
    void        set(const SceneObject* key, T* attrib) {
        set(key, rttr::type::get<T>(), attrib);
    }
    template<typename T>
    T*          get(const SceneObject* key) {
        return (T*)get(key, rttr::type::get<T>());
    }

    void        eraseAll(const SceneObject* key) {
        data.erase(key);
    }
    void        erase(const SceneObject* key, rttr::type t) {
        data[key].erase(t);
    }
    void        set(const SceneObject* key, rttr::type t, Attribute* attrib) {
        data[key][t].reset(attrib);
    }
    Attribute*  get(const SceneObject* key, rttr::type t) {
        auto it = data.find(key);
        if(it == data.end()) return 0;
        auto it2 = it->second.find(t);
        if(it2 == it->second.end()) return 0;
        return it2->second.get();
    }

private:
    std::map<
        const SceneObject*,
        std::map<
            rttr::type,
            std::shared_ptr<Attribute>
        >
    >           data;
};

static Factory<const SceneObject*, Behavior> bhvr_factory;
static AttribFactory attrib_factory;

SceneObject::~SceneObject() {
    bhvr_factory.erase(this);
    attrib_factory.eraseAll(this);
}

void SceneObject::setBehavior(rttr::type t) {
    Behavior* b = t.create().get_value<Behavior*>();
    if(!b) return;
    b->object = this;

    bhvr_factory.set(this, b);
}
Behavior* SceneObject::getBehavior() const {
    return bhvr_factory.get(this);
}
void SceneObject::eraseBehavior() {
    bhvr_factory.erase(this);
}

Attribute* SceneObject::getAttrib(rttr::type t) {
    Attribute* a = attrib_factory.get(this, t);
    if(!a) {
        a = t.create().get_value<Attribute*>();
        if(!a) return 0;
        
        attrib_factory.set(this, t, a);
    }
    return a;
}
Attribute* SceneObject::findAttrib(rttr::type t) {
    return attrib_factory.get(this, t);
}
void SceneObject::eraseAttrib(rttr::type t) {
    attrib_factory.erase(this, t);
}
