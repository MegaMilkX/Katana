#ifndef ECS_GAME_SCENE_HPP
#define ECS_GAME_SCENE_HPP

#include <memory>

#include "resource/resource.h"
#include "ecs/world.hpp"

#include "ecs/attribs/model.hpp"

#include "../util/bullet_debug_draw.hpp"

#include "actor.hpp"

class ktGameWorld : public Resource {
    RTTR_ENABLE(Resource);

    std::unique_ptr<ecsWorld> ecs_world;

    std::set<ktActor*> actors;

public:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConf;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> constraintSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> bt_world;

    BulletDebugDrawer2_OpenGL debugDrawer;

    DebugDraw* debug_draw = 0;

    ktGameWorld() {
        ecs_world.reset(new ecsWorld);

        collisionConf.reset(new btDefaultCollisionConfiguration());
        dispatcher.reset(new btCollisionDispatcher(collisionConf.get()));
        broadphase.reset(new btDbvtBroadphase());
        constraintSolver.reset(new btSequentialImpulseConstraintSolver());
        bt_world.reset(new btDiscreteDynamicsWorld(
            dispatcher.get(), 
            broadphase.get(), 
            constraintSolver.get(),
            collisionConf.get()
        ));
        broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
        bt_world->setGravity(btVector3(0.0f, -10.0f, 0.0f));
        bt_world->setDebugDrawer(&debugDrawer);
    }
    ~ktGameWorld() {
    }

    void addActor(ktActor* actor) {
        actors.insert(actor);
        actor->onSpawn(this);
    }
    void removeActor(ktActor* actor) {
        actor->onDespawn(this);
        actors.erase(actor);
    }
    size_t actorCount() const {
        return actors.size();
    }
    ktActor* getActor(size_t idx) {
        auto it = actors.begin();
        std::advance(it, idx);
        return (*it);
    }

    void update(float dt) {
        for(auto a : actors) {
            a->onUpdate(dt);
        }
        
        bt_world->stepSimulation(dt);

        for(auto a : actors) {
            a->onPostCollisionUpdate(dt);
        }

        bt_world->debugDrawWorld();
    }

    void setDebugDraw(DebugDraw* dd) {
        debug_draw = dd;
        debugDrawer.setDD(dd);
    }

public:
    // Resource
    void    serialize(out_stream& out) override {
        // TODO
    }
    bool    deserialize(in_stream& in, size_t sz) override {
        // TODO
        return true;
    }
    const char* getWriteExtension() const override { return "world"; }
};



class ImGuiPropertyView {
public:
    virtual ~ImGuiPropertyView() {}
    virtual void set(rttr::instance& inst, rttr::property& prop) = 0;
    virtual void get(rttr::instance& inst) = 0;
    virtual bool onGui() {
        ImGui::Text("Unsupported property type");
        return false;
    }
};

template<typename T>
class ImGuiPropertyViewTpl : public ImGuiPropertyView {
protected:
    std::string name;
    rttr::property property = rttr::detail::create_invalid_item<rttr::property>();
    T value;
    void set(rttr::instance& inst, rttr::property& prop) override {
        name = prop.get_name().to_string();
        property = prop;
        rttr::variant var = prop.get_value(inst);
        value = var.get_value<T>();
        onSet(value);
    }
    void get(rttr::instance& inst) override {
        value = onGet();
        property.set_value(inst, rttr::variant(value));
    }
    virtual void onSet(const T& val) { value = val; }
    virtual T onGet() const { return value; }
    virtual bool onGui() override {
        ImGui::Text("Unsupported property type (template)");
        return false;
    }
};

class ImGuiPropCString : public ImGuiPropertyViewTpl<const char*> {
    std::string str;
    void onSet(const char* const& val) override {
        str = val;
    }
    const char* onGet() const override {
        return str.c_str();
    }
    bool onGui() override {
        char buf[512];
        memset(buf, 0, 512);
        memcpy(buf, str.c_str(), str.size());
        if(ImGui::InputText(name.c_str(), buf, 512)) {
            str = buf;
            return true;
        }
        return false;
    }
};
class ImGuiPropVec3 : public ImGuiPropertyViewTpl<gfxm::vec3> {
    bool onGui() override {
        if(ImGui::DragFloat3(name.c_str(), (float*)&value)) {
            return true;
        }
        return false;
    }
};
class ImGuiPropQuat : public ImGuiPropertyViewTpl<gfxm::quat> {
    gfxm::vec3 euler;
    void onSet(const gfxm::quat& val) override {
        euler = gfxm::to_euler(val);
        euler.x = gfxm::degrees(euler.x);
        euler.y = gfxm::degrees(euler.y);
        euler.z = gfxm::degrees(euler.z);
    }
    gfxm::quat onGet() const override {
        return gfxm::euler_to_quat(gfxm::vec3(gfxm::radian(euler.x), gfxm::radian(euler.y), gfxm::radian(euler.z)));
    }
    bool onGui() override {
        if(ImGui::DragFloat3(name.c_str(), (float*)&euler)) {
            return true;
        }
        return false;
    }
};

template<typename T>
ImGuiPropertyView* propertyViewConstructor(rttr::instance& inst, rttr::property& prop) {
    auto ptr = new T();
    ((ImGuiPropertyView*)ptr)->set(inst, prop);
    return ptr;
}

std::map<rttr::type, ImGuiPropertyView*(*)(rttr::instance&, rttr::property&)>& getImGuiPropertyConstructorMap() {
    static std::map<rttr::type, ImGuiPropertyView*(*)(rttr::instance&, rttr::property&)> map;
    return map;
}

template<typename PROP_T, typename VIEW_T>
void regProperty() {
    getImGuiPropertyConstructorMap()[rttr::type::get<PROP_T>()] = &propertyViewConstructor<VIEW_T>;
}

void initImGuiProperties() {
    regProperty<const char*, ImGuiPropCString>();
    regProperty<gfxm::vec3, ImGuiPropVec3>();
    regProperty<gfxm::quat, ImGuiPropQuat>();
}

std::shared_ptr<ImGuiPropertyView> createImGuiPropertyView(rttr::instance& inst, rttr::property& prop) {
    auto& m = getImGuiPropertyConstructorMap();
    auto it = m.find(prop.get_type());
    if(it == m.end()) {
        return std::shared_ptr<ImGuiPropertyView>(0);
    } else {
        return std::shared_ptr<ImGuiPropertyView>(it->second(inst, prop));
    }
}


STATIC_RUN(ktGameWorld) {
    rttr::registration::class_<ecsWorld>("GameScene")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );

    // Move to Init
    rttr::registration::class_<ktActor>("Actor")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr)
        .property("Name", &ktActor::getName, &ktActor::setName)
        .property("Translation", &ktActor::getTranslation, &ktActor::setTranslation)
        .property("Rotation", &ktActor::getRotation, &ktActor::setRotation)
        .property("Scale", &ktActor::getScale, &ktActor::setScale);


    initImGuiProperties();
}

#endif

