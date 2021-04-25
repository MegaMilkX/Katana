#ifndef KT_ACTOR_STATIC_MODEL_HPP
#define KT_ACTOR_STATIC_MODEL_HPP

#include "actor.hpp"
#include "game_world.hpp"

#include "../resource/static_model.hpp"
#include "../util/assimp_scene.hpp"


class ktAStaticModel : public ktActor {
    RTTR_ENABLE(ktActor)

    std::shared_ptr<StaticModel> static_mesh;
public:
    void setModel(std::shared_ptr<StaticModel> model) {

    }

    // ktActor
    void onSpawn(ktGameWorld* world) override {
        
    }
    void onDespawn(ktGameWorld* world) override {
        
    }

    void onGui() override {
        ktActor::onGui();
        imguiResourceTreeCombo("static mesh", static_mesh, "static_mesh", [](){

        },[this](ResourceNode* node){
            if(node->getExtension().compare("fbx") == 0) {
                AssimpScene scn;
                auto buf = node->readBytes();
                scn.load(buf.data(), buf.size(), node->getName());
                auto ai_scene = scn.getScene();
                
                
            }
        });
    }
};
STATIC_RUN(ktAStaticModel) {
    rttr::registration::class_<ktAStaticModel>("StaticModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif

