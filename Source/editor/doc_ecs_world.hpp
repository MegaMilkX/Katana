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
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_signature() const override {
        return get_signature_static();
    }
};


class ecsTestArchetype : public ecsArchetype<ecsTranslation, ecsVelocity> {
public:

};

struct ecsaMovable {
    ecsTranslation* translation;
    ecsVelocity* velocity;
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

class ecsSystemBase {
public:
    virtual ~ecsSystemBase() {}
    virtual bool tryFit(uint64_t entity_sig) = 0;

    virtual void onUpdate() {

    }
};

template<typename... Args>
class ecsSystem;

template<typename Arg>
class ecsSystem<Arg> : public ecsSystemBase {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    bool tryFit(uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            // TODO: FIT
            return true;
        }
    }
};

template<typename Arg, typename... Args>
class ecsSystem<Arg, Args...> : public ecsSystem<Args...> {
public:
    virtual void onFit(Arg* arch) {}
    virtual void onUnfit(Arg* arch) {}

    bool tryFit(uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            // TODO: FIT
            return true;
        }
        return ecsSystem<Args...>::tryFit(entity_sig);
    }
};


class ecsTestSystem : public ecsSystem<ecsTestArchetype> {
public:
    void onUpdate() override {
        LOG("lol");
    }
};


template<typename STRUCT_T, typename... Args>
class ecsArchetypeConstructor {
    template<typename T>
    T* foo(ecsEntity* ent) {
        return ent->getAttrib<T>();
    }
public:
    STRUCT_T make(ecsEntity* ent) {
        return STRUCT_T { foo<Args>(ent)... };
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

        ecsArchetypeConstructor<ecsaMovable, ecsTranslation, ecsVelocity> con;
        auto movable = con.make(world.getEntity(ent));
    }

    void onGui(Editor* ed, float dt) override {
        //world.update();
    }
    void onGuiToolbox(Editor* ed) override {
    
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
