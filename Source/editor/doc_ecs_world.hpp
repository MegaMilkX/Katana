#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/util/singleton.hpp"

#include <assert.h>

#include "../common/ecs/bytepool.hpp"

#include "../common/util/attrib_type.hpp"

#include "../common/util/bitset.hpp"

#include "../common/util/object_pool.hpp"

#define ATTRIB_ENABLE \
    virtual attrib_type get_attrib_type() const { \
        return attrib_type::get<decltype(*this)>(); \
    }

uint64_t next_attrib_id() {
    static uint64_t id = 0;
    return id++;
}

class ecsAttribBase {
public:
    virtual ~ecsAttribBase() {}
    virtual uint64_t get_id() const = 0;
};

template<typename T>
class ecsAttrib : public ecsAttribBase {
public:
    static uint64_t get_id_static() {
        static uint64_t id = next_attrib_id();
        return id;
    } 
    uint64_t get_id() const override {
        return get_id_static();
    }
};

class ecsTranslation : public ecsAttrib<ecsTranslation> {
public:
    gfxm::vec3 translation;
};
class ecsRotation : public ecsAttrib<ecsRotation> {
public:
    gfxm::quat rotation;
};
class ecsScale : public ecsAttrib<ecsScale> {
public:
    gfxm::vec3 scale;
};
class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
};
class ecsMass : public ecsAttrib<ecsMass> {
public:
    float mass;
};


typedef size_t entity_id;

class ecsEntity {
    uint64_t attrib_bits;
    std::unordered_map<uint8_t, std::shared_ptr<ecsAttribBase>> attribs;
public:
    template<typename T>
    T* getAttrib() {
        auto a = new T();
        attribs[a->get_id()].reset(a);
        return a;
    }

    const uint64_t& getAttribBits() const {
        return attrib_bits;
    }
    void setBit(attrib_id attrib) {
        attrib_bits |= (1 << attrib);
    }
    void clearBit(attrib_id attrib) {
        attrib_bits &= ~(1 << attrib);
    }
};


class ecsArchetypeBase {
public:
    virtual ~ecsArchetypeBase() {}
    virtual uint64_t get_signature() const = 0;
};
template<typename... Args>
class ecsArchetype : public ecsArchetypeBase {
public:
    static uint64_t get_signature_static() {
        static uint64_t x[] = { Args::get_id_static()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= (1 << x[i]);
        }
        return sig;
    }
    uint64_t get_signature() const override {
        return get_signature_static();
    }

    template<typename T>
    T makeStruct(ecsEntity* ent) {
        return T { ent->getAttrib<Args>()... };
    }

};

class ecsMovableArchetype : public ecsArchetype<ecsTranslation, ecsVelocity> {
public:

};

struct ecsaMovable {
    ecsTranslation* translation;
    ecsVelocity* velocity;
};


class ecsSystemBase {
public:
    virtual ~ecsSystemBase() {}
    virtual bool tryFit(entity_id ent, uint64_t entity_sig) = 0;

    virtual void onUpdate() {

    }
};


template<typename T>
class ecsArchetypeMap {
protected:
    std::map<entity_id, T> values;
public:
    void insert(entity_id ent, const T& arch) {
        LOG("insert " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        values[ent] = arch;
    }
};

template<typename... Args>
class ecsSystemRecursive;

template<typename Arg>
class ecsSystemRecursive<Arg> 
: public ecsArchetypeMap<Arg>, public ecsSystemBase {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    bool tryFit(entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            ecsArchetypeMap<Arg>::insert(ent, Arg());
            return true;
        }
    }
};

template<typename Arg, typename... Args>
class ecsSystemRecursive<Arg, Args...> 
: public ecsArchetypeMap<Arg>, public ecsSystemRecursive<Args...> {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    bool tryFit(entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            ecsArchetypeMap<Arg>::insert(ent, Arg());
            return true;
        }
        return ecsSystemRecursive<Args...>::tryFit(ent, entity_sig);
    }
};

template<typename... Args>
class ecsSystem : public ecsSystemRecursive<Args...> {
public:
    template<typename ARCH_T>
    std::map<entity_id, ARCH_T>& get_archetype_map() {
        return ecsArchetypeMap<ARCH_T>::values;
    }
};


class ecsTestSystem : public ecsSystem<
    ecsArchetype<ecsTranslation, ecsVelocity>,
    ecsArchetype<ecsTranslation, ecsMass>
> {
public:
    void onUpdate() {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsVelocity>>()) {
            LOG(kv.first << ": Moving");
        }
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsMass>>()) {
            LOG(kv.first << ": Falling");
        }
    }
};


class ecsWorld {
    ObjectPool<ecsEntity> entities;
    std::vector<std::unique_ptr<ecsSystemBase>> systems;

    bool fitsSignature(uint64_t sig, uint64_t entity_bits) {
        return (sig & entity_bits) == sig;
    }

public:
    entity_id createEntity() {
        return entities.acquire();
    }
    void removeEntity(entity_id id) {
        entities.free(id);
    }
    ecsEntity* getEntity(entity_id id) {
        return entities.deref(id);
    }

    template<typename T>
    void setAttrib(entity_id ent) {
        auto e = entities.deref(ent);
        auto a = e->getAttrib<T>();
        e->setBit(a->get_id());
        for(auto& sys : systems) {
            sys->tryFit(ent, e->getAttribBits());
        }
    }

    template<typename T>
    void addSystem() {
        systems.push_back(std::unique_ptr<ecsSystemBase>(new T()));
    }

    void update() {
        for(auto& sys : systems) {
            sys->onUpdate();
        }
    }
};


class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
public:
    DocEcsWorld() {
        world.addSystem<ecsTestSystem>();
        auto ent = world.createEntity();
        world.setAttrib<ecsTranslation>(ent);
        world.setAttrib<ecsVelocity>(ent);

        ecsMovableArchetype movable_arch;
        auto movable = movable_arch.makeStruct<ecsaMovable>(world.getEntity(ent));


    }

    void onGui(Editor* ed, float dt) override {
        world.update();
    }
    void onGuiToolbox(Editor* ed) override {
        if(ImGui::Button("Add entity TRANS+VELO")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTranslation>(ent);
            world.setAttrib<ecsVelocity>(ent);
        }
        if(ImGui::Button("Add entity TRANS+MASS")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTranslation>(ent);
            world.setAttrib<ecsMass>(ent);
        }
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
