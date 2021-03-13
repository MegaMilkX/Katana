#ifndef KT_GAME_WORLD_HPP
#define KT_GAME_WORLD_HPP

#include <functional>

#include "../ecs/world.hpp"
#include "render_scene.hpp"
#include "dynamics_world.hpp"
#include "../resource/model/model.hpp"

#include "game_object/component/component.hpp"
#include "game_object/actor.hpp"

#include "archetype/archetype.hpp"

class ktActorStorage {
public:
};

class ktGameWorld : public Resource {
    RTTR_ENABLE(Resource);

    ktRenderScene   render_scene;
    ktDynamicsWorld dynamics_world;

    ktArchetypeGraph archetype_graph;

    std::unordered_map<rttr::type, std::unique_ptr<ktArchetype>> actor_archetypes;

    std::vector<std::pair<rttr::type, int>>* constructed_signature;

    template<typename... Args>
    void for_each_entity(const std::function<void(Args*...)>& fn) {
        auto signature = make_archetype_signature<Args...>();
        for(auto& a : archetype_graph.getArchetypes()) {
            if(!ktSignatureContainsAll(a->signature, signature)) {
                continue;
            }
            for(int i = 0; i < a->count(); ++i) {
                fn(((Args*)a->deref(i, rttr::type::get<Args>()))...);
            }
        }
        for(auto& kv : actor_archetypes) {
            if(!ktSignatureContainsAll(kv.second->signature, signature)) {
                continue;
            }
            for(int i = 0; i < kv.second->count(); ++i) {
                fn(((Args*)kv.second->deref(i, rttr::type::get<Args>()))...);
            }
        }
    }
    template<typename T>
    void for_each_action(const std::function<void(T*)>& fn) {
        
    }
    template<typename... Args>
    void add_constructor(const std::function<void(Args*...)>& fn) {
        auto signature = make_archetype_signature<Args...>();
        auto arch = archetype_graph.getArchetype(signature);
        arch->addConstructor(fn);
    }
    template<typename... Args>
    void add_destructor(const std::function<void(Args*...)>& fn) {
        
    }
    template<typename... Args>
    void add_member_function(const std::string& name, const std::function<void(Args*...)>& fn) {

    }


public:
    std::set<ktGameObject*> game_objects;

    ktGameWorld() {
        add_constructor<ktCoolComponent, ktVeryCoolComponent>([](ktCoolComponent* cool, ktVeryCoolComponent* very_cool){
            LOG_WARN("Constructor invoked");
            cool->my_value = 13.0f;
            very_cool->cool_value = 13.0f;
        });
        add_member_function<ktCoolComponent, ktVeryCoolComponent>("Ping", [](ktCoolComponent* cool, ktVeryCoolComponent* very_cool){

        });
    }
    ~ktGameWorld() {
        for(auto o : game_objects) {
            delete o;
        }
    }

    ktArchetypeGraph* getArchetypeGraph() { return &archetype_graph; }
    ktRenderScene* getRenderScene() { return &render_scene; }
    ktDynamicsWorld* getDynamicsWorld() { return &dynamics_world; }

    template<typename GAME_OBJECT_T>
    GAME_OBJECT_T* createObject() {
        static_assert(std::is_base_of<ktGameObject, GAME_OBJECT_T>::value, "GAME_OBJECT_T must be derived from ktGameObject");

        auto& arch = actor_archetypes[rttr::type::get<GAME_OBJECT_T>()];
        if(!arch) {
            arch.reset(new ktArchetype(sizeof(GAME_OBJECT_T)));
            int idx = arch->allocOne();
            void* base_ptr = arch->derefBasePtr(idx);
            std::vector<std::pair<rttr::type, int>> signature;
            constructed_signature = &signature;
            new (base_ptr) GAME_OBJECT_T(this);
            constructed_signature = 0;
            ktSignatureSort(signature);
            arch->_lateSignatureInit(signature);
            arch->invokeConstructors(idx);
            game_objects.insert((GAME_OBJECT_T*)base_ptr);
            return (GAME_OBJECT_T*)base_ptr;
        } else {
            int idx = arch->allocOne();
            void* base_ptr = arch->derefBasePtr(idx);
            new (base_ptr) GAME_OBJECT_T(this);
            arch->invokeConstructors(idx);
            game_objects.insert((GAME_OBJECT_T*)base_ptr);
            return (GAME_OBJECT_T*)base_ptr;
        }
        //ktGameObject* o = new GAME_OBJECT_T(this);
        //game_objects.insert(o);
        //return (GAME_OBJECT_T*)o;
    }
    void removeObject(ktGameObject* object) {
        game_objects.erase(object);
        delete object;
    }

    template<typename COMPONENT_T>
    void enableComponent(ktEntity* entity_ptr, COMPONENT_T* member_ptr) {
        if(!constructed_signature) {
            return;
        }
        constructed_signature->push_back(std::make_pair(rttr::type::get<COMPONENT_T>(), (int)( (uint8_t*)member_ptr - (uint8_t*)entity_ptr )));
    }

    template<typename COMPONENT_T>
    void createComponent(ktEntity* entity) {
        auto old_arch = entity->archetype;
        int old_arch_idx = entity->arch_array_index;
        auto new_arch = archetype_graph.add(old_arch, rttr::type::get<COMPONENT_T>());
        if(!new_arch) { // Attempted to create already existing component
            return;
        }
        int new_arch_idx = new_arch->allocOne();

        new_arch->copyFrom(old_arch, old_arch_idx, new_arch_idx);
        old_arch->freeOne(old_arch_idx);

        entity->archetype = new_arch;
        entity->arch_array_index = new_arch_idx;

        new_arch->invokeConstructors(new_arch_idx);
    }

    template<typename... Args>
    std::vector<rttr::type> make_archetype_signature() {
        std::vector<rttr::type> sig = { rttr::type::get<Args>()... };
        ktSignatureSort(sig);
        return sig;
    }

    void update(DebugDraw* dd) {
        for_each_entity<ktCoolComponent, ktVeryCoolComponent>([](ktCoolComponent* cool, ktVeryCoolComponent* very_cool){
            cool->my_value += very_cool->cool_value * 0.01f;
        });

        float dt = 1.0f/60.0f;

        dynamics_world.update(dt);
        render_scene.update();

        //render_scene.debugDraw(dd);
        dynamics_world.getDebugDraw()->setDD(dd);
        //dynamics_world.getBtWorld()->debugDrawWorld();
    }

};
STATIC_RUN(ktGameWorld) {
    rttr::registration::class_<ktGameWorld>("ktGameWorld")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}


#endif
