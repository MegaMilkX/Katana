#ifndef RENDER_CONTROLLER_HPP
#define RENDER_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../components/model.hpp"
#include "../../components/camera.hpp"

#include "../../draw_list.hpp"

#include <set>

#include "../../../common/util/imgui_helpers.hpp"

#include "../scene_listener.hpp"

class RenderController : public SceneController, public SceneListener {
    RTTR_ENABLE(SceneController)
public:
    virtual void init(GameScene* s) {
        scene = s;
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_REMOVED);
    }
    ~RenderController() {
        scene->getEventMgr().unsubscribeAll(this);
    }

    virtual void copy(SceneController& other) {
        RenderController& o = (RenderController&)other;
        if(o.default_camera) {
            std::string cam_name = o.default_camera->getOwner()->getName();
            auto obj = scene->findObject(cam_name);
            if(obj) {
                auto c = obj->find<CmCamera>();
                if(c) {
                    default_camera = c.get();
                } else {
                    LOG_WARN("Default camera object has no camera component");
                }
            } else {
                LOG_WARN("Failed to find default camera object");
            }
        }
    }

    void getDrawList(DrawList& dl) {
        for(auto m : models) {
            for(size_t i = 0; i < m->segmentCount(); ++i) {
                if(!m->getSegment(i).mesh) continue;
                if(!m->getSegment(i).skin_data) {
                    dl.add(DrawList::Solid{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).material.get(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform()
                    });
                } else {
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : m->getSegment(i).skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(t->getTransform()->getWorldTransform());
                        } else {
                            bone_transforms.emplace_back(gfxm::mat4(1.0f));
                        }
                    }
                    dl.add(DrawList::Skin{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).material.get(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform(),
                        bone_transforms,
                        m->getSegment(i).skin_data->bind_transforms
                    });
                }
            }
        }
        for(auto l : omnis) {
            dl.add(l);
        }
    }

    CmCamera* getDefaultCamera() {
        return default_camera;
    }
    void setDefaultCamera(CmCamera* c) {
        default_camera = c;
    }

    void _regModel(CmModel* m) {
        models.insert(m);
    }
    void _unregModel(CmModel* m) {
        models.erase(m);
    }

    void _regOmniLight(OmniLight* l) {
        omnis.insert(l);
    }
    void _unregOmniLight(OmniLight* l) {
        omnis.erase(l);
    }

    virtual void onGui() {
        imguiComponentCombo(
            "Default camera",
            default_camera,
            scene
        );
    }

    virtual void serialize(out_stream& out) {
        DataWriter w(&out);
        if(default_camera) {
            w.write(default_camera->getOwner()->getName());
        } else {
            w.write(std::string());
        }
    }
    virtual void deserialize(in_stream& in) {
        DataReader r(&in);
        default_camera = scene->findComponent<CmCamera>(r.readStr());
    }
private:
    virtual void onComponentRemoved(ObjectComponent* c) {
        if(c == default_camera) {
            default_camera = 0;
        }
    }

    std::set<CmModel*> models;
    std::set<OmniLight*> omnis;
    CmCamera* default_camera = 0;
    GameScene* scene = 0;
};
STATIC_RUN(RenderController) {
    rttr::registration::class_<RenderController>("RenderController")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
