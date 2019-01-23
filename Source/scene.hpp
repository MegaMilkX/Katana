#ifndef SCENE_HPP
#define SCENE_HPP

#include "handle_mgr.hpp"
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "scene_object.hpp"
#include "resource/resource.h"
#include "util/log.hpp"

// Required components
#include "camera.hpp"

class ISceneProbeBase {
public:
    virtual ~ISceneProbeBase() {}
    virtual void onCreateComponent(Component* c) = 0;
    virtual void onRemoveComponent(Component* c) = 0;
};

template<typename T>
class ISceneProbe : public ISceneProbeBase {
public:
    virtual ~ISceneProbe() {}
    virtual void onCreateComponent(Component* c) {
        onCreateComponent((T*)c);
    }
    virtual void onRemoveComponent(Component* c) {
        onRemoveComponent((T*)c);
    }
    virtual void onCreateComponent(T* c) = 0;
    virtual void onRemoveComponent(T* c) = 0;
};

class Scene {
    friend SceneObject;
public:
    static Scene* create() {
        return new Scene();
    }
    void destroy() {
        clear();
        delete this;
    }

    void clear() {
        // Root object always remains
        while(objectCount() > 1) {
            removeObject(getObject(objectCount() - 1));
        }
        getRootObject()->removeComponents();
        local_resources.clear();
    }

    SceneObject* createObject(SceneObject* parent = 0) {
        if(!parent) {
            parent = getRootObject();
        }
        objects.emplace_back(std::shared_ptr<SceneObject>(new SceneObject(this, parent)));
        if(parent) {
            parent->children.emplace_back(objects.back().get());
        }
        objects.back()->id = objects.size() - 1;
        return objects.back().get();
    }
    void removeObject(SceneObject* so) {
        if(!so->parent) {
            // Root object always remains
            return;
        }
        auto it = objects.end();
        for(size_t i = 0; i < objects.size(); ++i) {
            // Note the one to remove
            if(objects[i].get() == so) {
                it = objects.begin() + i;
                continue;
            }
            // If it is a parent to this object, clear the relationship
            if(objects[i]->getParent() == so) {
                objects[i]->setParent(getRootObject());
            }
        }
        if(it != objects.end()) {
            SceneObject* parent = (*it)->getParent();
            if(parent) {
                parent->removeChild((*it).get());
            }
            (*it)->removeComponents();
            objects.erase(it);
        }
    }
    size_t objectCount() const {
        return objects.size();
    }
    SceneObject* getObject(size_t index) {
        return objects[index].get();
    }
    SceneObject* getRootObject() {
        return objects[0].get();
    }

    Camera* getCurrentCamera() { return current_camera; }
    void    setCurrentCamera(Camera* cam) { 
        current_camera = cam;
        LOG("Set current camera: " << current_camera); 
    }

    template<typename T>
    T* createComponent(SceneObject* so = 0) {
        components[rttr::type::get<T>()].emplace_back(std::shared_ptr<Component>(new T()));
        T* c = (T*)components[rttr::type::get<T>()].back().get();
        c->scene_object = so;
        c->id = (components[rttr::type::get<T>()].size() - 1);
        c->onCreate();
        triggerProbeOnCreateRecursive(rttr::type::get<T>(), c);
        return c;
    }
    Component* createComponent(const std::string& type_name, SceneObject* so = 0) {
        rttr::type type = rttr::type::get_by_name(type_name);
        if(!type.is_valid()) {
            LOG_WARN(type_name << " is not a valid type");
            return 0;
        }
        rttr::variant v = type.create();
        if(!v.get_type().is_pointer()) {
            LOG_WARN(type_name << " - rttr constructor policy must be pointer");
            return 0;
        }
        Component* c = v.get_value<Component*>();
        if(c) {
            components[type].emplace_back(std::shared_ptr<Component>(c));
            c->scene_object = so;
            c->id = (components[type].size() - 1);
            c->onCreate();
            triggerProbeOnCreateRecursive(type, c);
        }
        
        return c;
    }
    Component* createComponentCopy(rttr::type type, Component* original, SceneObject* owner = 0) {
        Component* cpy = original->clone();
        if(!cpy) return 0;

        components[type].emplace_back(std::shared_ptr<Component>(cpy));
        cpy->scene_object = owner;
        cpy->id = (components[type].size() - 1);
        return cpy;
    }
    template<typename T>
    T* getComponent(uint64_t id) {
        return (T*)components[rttr::type::get<T>()][id].get();
    }
    template<typename T>
    void removeComponent(T* c) {
        removeComponent(rttr::type::get<T>(), c);
    }
    void removeComponent(rttr::type type, Component* c) {
        std::vector<std::shared_ptr<Component>>& vec = 
            components[type];
        for(size_t i = 0; i < vec.size(); ++i) {
            if(vec[i].get() == c) {
                triggerProbeOnRemoveRecursive(type, c);
                vec.erase(vec.begin() + i);
                return;
            }
        }
    }

    void copyObjectRecursive(SceneObject* to, SceneObject* from) {
        to->setName(from->getName());
        LOG("Copying " << from->getName());
        for(size_t i = 0; i < from->childCount(); ++i) {
            SceneObject* new_so = to->createChild();
            copyObjectRecursive(new_so, from->getChild(i));
        }
        to->cloneFrom(from);
    }

    void copyFrom(Scene* other) {
        LOG("Copying from scene " << other);
        clear();
        //copyObjectRecursive(getRootObject(), other->getRootObject());

        // Objects
        for(size_t i = 1; i < other->objects.size(); ++i) {
            createObject();
        }
        for(size_t i = 0; i < other->objects.size(); ++i) {
            SceneObject* other_so = other->objects[i].get();
            SceneObject* so = objects[i].get();
            if(other_so->parent) {
                size_t parent_id = other_so->parent->getId();
                so->parent = objects[parent_id].get();
            }
            for(size_t j = 0; j < other_so->children.size(); ++j) {
                size_t child_id = other_so->children[j]->getId();
                so->children.emplace_back(objects[child_id].get());
            }
        }

        // Components
        for(auto& kv : other->components) {
            for(size_t i = 0; i < kv.second.size(); ++i) {
                size_t owner_id = kv.second[i]->scene_object->getId();
                SceneObject* owner_so = objects[owner_id].get();
                Component* c = createComponentCopy(kv.first, kv.second[i].get(), owner_so);
                owner_so->components[kv.first] = c;
            }
        }
        for(auto& kv : components) {
            for(size_t i = 0; i < kv.second.size(); ++i) {
                kv.second[i]->onCreate();
                triggerProbeOnCreateRecursive(kv.first, kv.second[i].get());
            }
        }

        local_resources = other->local_resources;

        LOG("Done");
    }

    template<typename T>
    void addLocalResource(std::shared_ptr<T> ptr) {
        addLocalResource(rttr::type::get<T>(), std::dynamic_pointer_cast<Resource>(ptr));
    }
    void addLocalResource(rttr::type type, std::shared_ptr<Resource> res) {
        local_resources[type].emplace_back(res);
    }
    template<typename T>
    std::shared_ptr<T> getLocalResource(size_t index) {
        auto& vec = local_resources[rttr::type::get<T>()];

        LOG("Local " << rttr::type::get<T>().get_name().to_string() << " resource requested: " << index);
        LOG("Total " << rttr::type::get<T>().get_name().to_string() << " resources: " << local_resources[rttr::type::get<T>()].size());
        
        if(index >= vec.size()) {
            return std::shared_ptr<T>();
        }
        
        return std::dynamic_pointer_cast<T>(vec[index]);
    }
    template<typename T>
    size_t getLocalResourceId(std::shared_ptr<T> ptr) {
        std::shared_ptr<Resource> pt = std::dynamic_pointer_cast<Resource>(ptr);
        size_t i = 0;
        for(auto p : local_resources[rttr::type::get<T>()]) {
            if(p.get() == pt.get()) {
                return i; 
            }
            ++i;
        }
        return (size_t)-1;
    }
    template<typename T>
    size_t localResourceCount() {
        return local_resources[rttr::type::get<T>()].size();
    }

    template<typename T>
    void setProbe(ISceneProbe<T>* probe_ptr) {
        rttr::type t = rttr::type::get<T>();
        removeProbe<T>();

        probes[t] = probe_ptr;
        std::vector<std::shared_ptr<Component>>& comps = 
            components[t];
        for(auto& p : comps) {
            probe_ptr->onCreateComponent((T*)p.get());
        }
    }
    template<typename T>
    void removeProbe() {
        rttr::type t = rttr::type::get<T>();
        ISceneProbeBase* probe = probes[t];
        if(!probe) return;

        std::vector<std::shared_ptr<Component>>& comps = 
            components[t];
        for(auto& p : comps) {
            probe->onRemoveComponent(p.get());
        }
        probes.erase(rttr::type::get<T>());
    }

    bool serialize(std::vector<char>& buf);
    bool serialize(const std::string& filename);
    bool deserialize(std::vector<char>& buf);
    bool deserialize(const std::string& filename);

    void _editorGuiComponentList() {
        int i = 0;
        for(auto kv : components) {
            bool node_open = ImGui::TreeNodeEx(
                (void*)i, 0, kv.first.get_name().to_string().c_str()
            );
            if(node_open) {
                for(auto& c : kv.second) {
                    ImGui::Text(MKSTR("Owner: " << c->getObject()).c_str());
                }  
                ImGui::TreePop();
            }
            ++i;
        }
    }
    void _editorGuiResourceList() {
        int i = 0;
        for(auto kv : local_resources) {
            bool node_open = ImGui::TreeNodeEx(
                (void*)i, 0, kv.first.get_name().to_string().c_str()
            );
            if(node_open) {
                for(auto& r : kv.second) {
                    ImGui::Text(MKSTR(r->Name() << "##" << i).c_str());
                }  
                ImGui::TreePop();
            }
            ++i;
        }
    }
private:
    Scene() {
        objects.emplace_back(std::shared_ptr<SceneObject>(new SceneObject(this, 0)));
        getRootObject()->setName("Root");
        getRootObject()->id = 0;
    }
    Scene(const Scene& other) {}
    void operator=(const Scene& other) {}

    void triggerProbeOnCreateRecursive(rttr::type type, Component* c) {
        auto types = type.get_base_classes();
        for(auto& t : types) {
            triggerProbeOnCreateRecursive(t, c);
        }
        triggerProbeOnCreate(type, c);
    }
    void triggerProbeOnRemoveRecursive(rttr::type type, Component* c) {
        auto types = type.get_base_classes();
        for(auto& t : types) {
            triggerProbeOnRemoveRecursive(t, c);
        }
        triggerProbeOnRemove(type, c);
    }

    void triggerProbeOnCreate(rttr::type type, Component* c) {
        auto it = probes.find(type);
        if(it == probes.end()) return;
        it->second->onCreateComponent(c);
    }
    void triggerProbeOnRemove(rttr::type type, Component* c) {
        auto it = probes.find(type);
        if(it == probes.end()) return;
        it->second->onRemoveComponent(c);
    }

    std::vector<std::shared_ptr<SceneObject>> objects;
    std::map<rttr::type, std::vector<std::shared_ptr<Component>>> components;
    std::map<rttr::type, std::vector<std::shared_ptr<Resource>>> local_resources;

    std::map<rttr::type, ISceneProbeBase*> probes;

    Camera* current_camera = 0;
};

#endif
