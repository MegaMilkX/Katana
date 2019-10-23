#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/util/singleton.hpp"

#include <assert.h>
#include <tuple>

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
        T* ptr = 0;
        auto id = T::get_id_static();
        auto it = attribs.find(id); // Ok, since get_id() doesn't touch state and is not virtual
        if(it == attribs.end()) {
            ptr = new T();
            attribs[id].reset(ptr);
        } else {
            ptr = (T*)it->second.get();
        }
        return ptr;
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
template<typename Arg>
class ecsArchetypePart {
    Arg* ptr;
public:
    virtual ~ecsArchetypePart() {}
};
template<typename... Args>
class ecsArchetype : public ecsArchetypeBase {
    std::tuple<Args*...> attribs;
public:
    ecsArchetype() {}
    ecsArchetype(ecsEntity* ent) {
        attribs = std::tuple<Args*...>(ent->getAttrib<Args>()...);
    }

    template<typename T>
    T* get() {
        return std::get<T*>(attribs);
    }

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


class ecsWorld;

class ecsSystemBase {
public:
    virtual ~ecsSystemBase() {}
    virtual bool tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) = 0;

    virtual void onUpdate() {

    }
};


template<typename T>
class ecsArchetypeMap {
protected:
    std::map<entity_id, std::shared_ptr<T>> values;
public:
    T* insert(entity_id ent, const T& arch) {
        LOG("insert " << ent << ": " << rttr::type::get<T>().get_name().to_string());
        T* arch_ptr = new T(arch);
        values[ent].reset(arch_ptr);
        return arch_ptr;
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

    bool tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            auto ptr = ecsArchetypeMap<Arg>::insert(ent, Arg(world->getEntity(ent)));
            onFit(ptr);
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

    bool tryFit(ecsWorld* world, entity_id ent, uint64_t entity_sig) {
        uint64_t arch_sig = Arg::get_signature_static();
        if((arch_sig & entity_sig) == arch_sig) {
            auto ptr = ecsArchetypeMap<Arg>::insert(ent, Arg(world->getEntity(ent)));
            onFit(ptr);
            return true;
        }
        return ecsSystemRecursive<Args...>::tryFit(world, ent, entity_sig);
    }
};

template<typename... Args>
class ecsSystem : public ecsSystemRecursive<Args...> {
public:
    template<typename ARCH_T>
    std::map<entity_id, std::shared_ptr<ARCH_T>>& get_archetype_map() {
        return ecsArchetypeMap<ARCH_T>::values;
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
            sys->tryFit(this, ent, e->getAttribBits());
        }
    }

    template<typename T>
    T* addSystem() {
        T* sys = new T();
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        return sys;
    }

    void update() {
        for(auto& sys : systems) {
            sys->onUpdate();
        }
    }
};



class ecsTestSystem : public ecsSystem<
    ecsArchetype<ecsTranslation, ecsVelocity>,
    ecsArchetype<ecsTranslation, ecsMass>
> {
public:
    void onFit(ecsArchetype<ecsTranslation, ecsVelocity>* ptr) {
        ptr->get<ecsTranslation>()->translation = gfxm::vec3(rand() % 100 * .1f, rand() % 100 * .1f, rand() % 100 * .1f);
    }

    void onUpdate() {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsVelocity>>()) {
            gfxm::vec3& pos = kv.second->get<ecsTranslation>()->translation;
            gfxm::vec3& vel = kv.second->get<ecsVelocity>()->velo;

            vel.x += .01f;
            vel.y += .01f;
            float u = sinf(vel.x) * 10.0f;
            float v = cosf(vel.y) * 10.0f;
            
            pos = gfxm::vec3(
                (1 + v / 2 * cosf(u / 2)) * cosf(u), 
                (1 + v / 2 * cosf(u / 2)) * sinf(u), 
                v / 2 * sin(u / 2)
            );
        }
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsMass>>()) {
            //LOG(kv.first << ": Falling");
        }
    }
};

class ecsRenderSystem : public ecsSystem<
    ecsArchetype<ecsTranslation, ecsVelocity>
> {
    DrawList draw_list;
    gl::IndexedMesh mesh;
public:
    ecsRenderSystem() {
        makeSphere(&mesh, 0.5f, 6);
    }

    void onFit(ecsArchetype<ecsTranslation, ecsVelocity>* object) {
    }

    void fillDrawList(DrawList& dl) {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsVelocity>>()) {
            DrawCmdSolid cmd;
            cmd.indexOffset = 0;
            cmd.material = 0;
            cmd.object_ptr = 0;
            cmd.transform = gfxm::translate(gfxm::mat4(1.0f), kv.second->get<ecsTranslation>()->translation);
            cmd.vao = mesh.getVao();
            cmd.indexCount = mesh.getIndexCount();

            dl.solids.emplace_back(cmd);
        }
    }
};

#include "../common/gui_viewport.hpp"

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
    ecsRenderSystem* renderSys;

    GuiViewport gvp;

public:
    DocEcsWorld() {
        world.addSystem<ecsTestSystem>();
        renderSys = world.addSystem<ecsRenderSystem>();
        auto ent = world.createEntity();
        world.setAttrib<ecsTranslation>(ent);
        world.setAttrib<ecsVelocity>(ent);

        gvp.camMode(GuiViewport::CAM_ORBIT);
    }

    void onGui(Editor* ed, float dt) override {
        world.update();

        DrawList dl;
        renderSys->fillDrawList(dl);
        gvp.draw(dl);
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
