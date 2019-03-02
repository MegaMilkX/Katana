#ifndef ANIM_SCENE_MGR_HPP
#define ANIM_SCENE_MGR_HPP

#include "scene/game_scene.hpp"

#include "components/animation_stack.hpp"

class AnimationSceneMgr : public SceneListener {
public:
    ~AnimationSceneMgr() {
        if(scene) scene->getEventMgr().unsubscribeAll(this);
    }
    void setScene(GameScene* scene) {
        this->scene = scene;
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_CREATED);
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_REMOVED);
    }
    
    void update(float dt) {
        for(auto as : anim_stacks) {
            as->update(dt);
        }
    }
private:
    void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) {
        switch(e) {
        case EVT_COMPONENT_CREATED:
            if(payload.get_value<rttr::type>() == rttr::type::get<AnimationStack>()) {
                anim_stacks.insert(sender->get<AnimationStack>().get());
            }
            break;
        case EVT_COMPONENT_REMOVED:
            if(payload.get_value<rttr::type>() == rttr::type::get<AnimationStack>()) {
                anim_stacks.erase(sender->get<AnimationStack>().get());
            }
            break;
        };
    }

    GameScene* scene = 0;
    std::set<AnimationStack*> anim_stacks;
};

#endif
