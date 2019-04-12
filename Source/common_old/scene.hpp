#ifndef SCENE_HPP
#define SCENE_HPP

#include "handle_mgr.hpp"
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "scene_object.hpp"
#include "scene_components/scene_component.hpp"
#include "resource/resource.h"
#include "util/log.hpp"

// Required components
#include "camera.hpp"

#include "resource/cube_map.hpp"

#include "skybox_gradient.hpp"

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
    static Scene*   create();
    void            destroy();

    void            clear();

    SceneObject*    createObject(SceneObject* parent = 0);
    void            removeObject(SceneObject* so);
    size_t          objectCount() const;
    SceneObject*    getObject(size_t index);
    SceneObject*    getRootObject();

    // ====
    Camera* getCurrentCamera() { return current_camera; }
    void    setCurrentCamera(Camera* cam) { 
        current_camera = cam;
        LOG("Set current camera: " << current_camera); 
    }
    SkyboxGradient& getSkybox() {
        return skybox;
    }
    // ====

    template<typename T>
    T* getSceneComponent() {
        rttr::type t = rttr::type::get<T>();
        auto it = scene_components.find(t);
        if(it == scene_components.end()) {
            scene_components[t] =    
                std::shared_ptr<SceneComponent>(new T());
        }
        return (T*)scene_components[t].get();
    }
    SceneComponent* getSceneComponent(const std::string& type_name) {
        return getSceneComponent(rttr::type::get_by_name(type_name));
    }
    SceneComponent* getSceneComponent(rttr::type t) {
        auto it = scene_components.find(t);
        if(it == scene_components.end()) {
            if(!t.is_valid()) {
                LOG_WARN(t.get_name().to_string() << " is not a valid type");
                return 0;
            }
            rttr::variant v = t.create();
            if(!v.get_type().is_pointer()) {
                LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
                return 0;
            }
            SceneComponent* c = v.get_value<SceneComponent*>();
            if(c) {
                scene_components[t] =    
                    std::shared_ptr<SceneComponent>(c);
            }
        }
        return scene_components[t].get();
    }

    // ====

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
                refreshComponentIds(type);
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
        
        other->_logStats();

        // Objects
        for(size_t i = 1; i < other->objects.size(); ++i) {
            createObject();
        }
        for(size_t i = 0; i < other->objects.size(); ++i) {
            SceneObject* other_so = other->objects[i].get();
            SceneObject* so = objects[i].get();
            so->setName(other_so->getName());

            if(other_so->parent) {
                size_t parent_id = other_so->parent->getId();
                //so->setParent(objects[parent_id].get());
                so->parent = objects[parent_id].get();
            }
            for(size_t j = 0; j < other_so->children.size(); ++j) {
                size_t child_id = other_so->children[j]->getId();
                so->children.emplace_back(objects[child_id].get());
            }
        }

        // Components
        std::map<Component*, Component*> copy_pairs;
        for(auto& kv : other->components) {
            for(size_t i = 0; i < kv.second.size(); ++i) {
                size_t owner_id = kv.second[i]->scene_object->getId();
                SceneObject* owner_so = objects[owner_id].get();
                Component* c = createComponentCopy(kv.first, kv.second[i].get(), owner_so);
                owner_so->components[kv.first] = c;
                
                copy_pairs[c] = kv.second[i].get();
            }
        }

        for(auto& kv : copy_pairs) {
            kv.first->onCreate();
            kv.first->_onClone(kv.second);
        }
        
        for(auto& kv : components) {
            for(size_t i = 0; i < kv.second.size(); ++i) {
                triggerProbeOnCreateRecursive(kv.first, kv.second[i].get());
            }
        }

        local_resources = other->local_resources;

        _logStats();

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

    void _logStats() {
        LOG("=== Scene stats ===");
        LOG("Object count: " << objects.size());
        for(auto& kv : components) {
            LOG(kv.first.get_name().to_string() << " count: " << kv.second.size());
        }
        LOG("=== End ===");
    }

    void retriggerProbes(rttr::type t, Component* c) {
        triggerProbeOnRemoveRecursive(t, c);
        triggerProbeOnCreateRecursive(t, c);
    }

private:
    Scene() {
        objects.emplace_back(std::shared_ptr<SceneObject>(new SceneObject(this, 0)));
        getRootObject()->setName("Root");
        getRootObject()->id = 0;
    }
    Scene(const Scene& other) {}
    void operator=(const Scene& other) {}

    void refreshObjectIds() {
        for(size_t i = 0; i < objects.size(); ++i) {
            objects[i]->id = i;
        }
    }
    void refreshComponentIds(rttr::type type) {
        for(size_t i = 0; i < components[type].size(); ++i) {
            components[type][i]->id = i;
        }
    }

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

    std::map<rttr::type, std::shared_ptr<SceneComponent>> scene_components;

    std::map<rttr::type, ISceneProbeBase*> probes;

    Camera* current_camera = 0;

    // Environment
    std::shared_ptr<CubeMap> environment_map;
    SkyboxGradient skybox;
};

#endif
