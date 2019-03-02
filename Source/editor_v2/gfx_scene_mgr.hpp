#ifndef GFX_SCENE_HPP
#define GFX_SCENE_HPP

#include "scene/scene_listener.hpp"
#include "scene/game_scene.hpp"

#include "../common/util/log.hpp"

#include "gfx_draw_object.hpp"
#include "draw_list.hpp"

#include "components/model.hpp"

class GfxSceneMgr : public SceneListener {
public:
    ~GfxSceneMgr() {
        if(scene) scene->getEventMgr().unsubscribeAll(this);
    }
    void setScene(GameScene* scene) {
        this->scene = scene;
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_CREATED);
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_REMOVED);
    }

    void getDrawList(DrawList& dl) {
        for(auto m : models) {
            for(size_t i = 0; i < m->segmentCount(); ++i) {
                if(!m->getSegment(i).mesh) continue;
                if(!m->getSegment(i).skin_data) {
                    dl.add(DrawList::Solid{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform()
                    });
                } else {
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : m->getSegment(i).skin_data->bone_nodes) {
                        bone_transforms.emplace_back(t->getWorldTransform());
                    }
                    dl.add(DrawList::Skin{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform(),
                        bone_transforms,
                        m->getSegment(i).skin_data->bind_transforms
                    });
                }
            }
        }
    }
private:
    void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) {
        switch(e) {
        case EVT_OBJECT_CREATED:
            break;
        case EVT_OBJECT_REMOVED:
            break;
        case EVT_COMPONENT_CREATED:
            if(payload.get_value<rttr::type>() == rttr::type::get<CmModel>()) {
                models.insert(sender->get<CmModel>().get());
            }
            break;
        case EVT_COMPONENT_REMOVED:
            if(payload.get_value<rttr::type>() == rttr::type::get<CmModel>()) {
                models.erase(sender->get<CmModel>().get());
            }
            break;
        };
    }

    GameScene* scene = 0;
    std::set<GameObject*> objects;
    std::set<CmModel*> models;
};

#endif
