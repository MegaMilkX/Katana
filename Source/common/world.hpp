#ifndef KT_WORLD_HPP
#define KT_WORLD_HPP

#include "scene/controllers/render_controller.hpp"
#include "scene/controllers/dynamics_ctrl.hpp"
#include "scene/controllers/anim_controller.hpp"

#include "scene/game_scene.hpp"

#include "attributes/actors/actor.hpp"

class ktActor;
class ktWorld : public SceneEventFilter<ktActor> {
    bool started = false;

    std::unique_ptr<GameScene> scene;

    RenderController* render_ctrl;
    DynamicsCtrl* dynamics_ctrl;
    AnimController* anim_ctrl;

    std::set<ktActor*> actors;

    void onAttribCreated(ktActor* a);
    void onAttribRemoved(ktActor* a);
public:
    ktWorld()
    : scene(new GameScene()) {
        render_ctrl = scene->getController<RenderController>();
        dynamics_ctrl = scene->getController<DynamicsCtrl>();
        anim_ctrl = scene->getController<AnimController>();

        scene->addListener(this);
    }

    GameScene* getScene() {
        return scene.get();
    }
    RenderController* getRenderController() {
        return render_ctrl;
    }
    DynamicsCtrl* getPhysics() {
        return dynamics_ctrl;
    }

    void start();
    void update(float dt);
    void cleanup();
};

#endif
