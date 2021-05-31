#ifndef KT_ACTOR_STATIC_MODEL_HPP
#define KT_ACTOR_STATIC_MODEL_HPP

#include "actor.hpp"
#include "game_world.hpp"

#include "../resource/static_model.hpp"
#include "../util/assimp_scene.hpp"


class ktAStaticModel : public ktActor {
    RTTR_ENABLE(ktActor)

    std::shared_ptr<StaticModel> static_mesh;

    
    ktGameWorld* world = 0;
public:
    void setModel(std::shared_ptr<StaticModel> model) {

    }

    // ktActor
    void onSpawn(ktGameWorld* world) override {
        this->world = world;
    }
    void onDespawn(ktGameWorld* world) override {
        this->world = 0;
    }

    void onUpdate(float dt) override {
        world->debug_draw->point(getWorldTransform() * gfxm::vec4(0.f,0.f,0.f,1.f), gfxm::vec3(1, 0, 0));
        world->debug_draw->circle(getWorldTransform() * gfxm::vec4(0.f,0.f,0.f,1.f), 0.5f, gfxm::vec3(1,0,0), gfxm::to_mat3(getWorldTransform()));
    }
};
STATIC_RUN(ktAStaticModel) {
    rttr::registration::class_<ktAStaticModel>("StaticModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif

