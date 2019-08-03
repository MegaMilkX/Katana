#ifndef RENDER_CONTROLLER_HPP
#define RENDER_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../components/model.hpp"
#include "../../components/camera.hpp"
#include "../../components/light_source.hpp"

#include "../../../common/draw_list.hpp"

#include <set>

#include "../../../common/util/imgui_helpers.hpp"

class RenderController : public SceneControllerEventFilter<Model, Camera, OmniLight> {
    RTTR_ENABLE(SceneController)
public:
    virtual void init(GameScene* s) {
        scene = s;
    }

    virtual void onAttribCreated(Model* m) { models.insert(m); }
    virtual void onAttribRemoved(Model* m) { models.erase(m); } 
    virtual void onAttribCreated(OmniLight* l) { omnis.insert(l); }
    virtual void onAttribRemoved(OmniLight* l) { omnis.erase(l); }
    virtual void onAttribCreated(Camera* c) {
        if(!default_camera) {
            default_camera = c;
        }
    }
    virtual void onAttribRemoved(Camera* c) {
        if(c == default_camera) {
            default_camera = 0;
        }
    }

    void getDrawList(DrawList& dl) {
        for(auto m : models) {
            if(!m->getOwner()->isEnabled()) {
                continue;
            }
            for(size_t i = 0; i < m->segmentCount(); ++i) {
                if(!m->getSegment(i).mesh) continue;
                if(!m->getSegment(i).skin_data) {
                    DrawCmdSolid s;
                    s.vao = m->getSegment(i).mesh->mesh.getVao();
                    s.material = m->getSegment(i).material.get();
                    s.indexCount = m->getSegment(i).mesh->mesh.getIndexCount();
                    s.transform = m->getOwner()->getTransform()->getWorldTransform();
                    dl.solids.emplace_back(s);
                    /*
                    dl.add(DrawList::Solid{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).material.get(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform()
                    });*/
                } else {
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : m->getSegment(i).skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(t->getTransform()->getWorldTransform());
                        } else {
                            bone_transforms.emplace_back(gfxm::mat4(1.0f));
                        }
                    }
                    DrawCmdSkin s;
                    s.vao = m->getSegment(i).mesh->mesh.getVao();
                    s.material = m->getSegment(i).material.get();
                    s.indexCount = m->getSegment(i).mesh->mesh.getIndexCount();
                    s.transform = m->getOwner()->getTransform()->getWorldTransform();
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = m->getSegment(i).skin_data->bind_transforms;
                    dl.skins.emplace_back(s);
                    /*
                    dl.add(DrawList::Skin{
                        m->getSegment(i).mesh->mesh.getVao(),
                        m->getSegment(i).material.get(),
                        m->getSegment(i).mesh->mesh.getIndexCount(),
                        m->getOwner()->getTransform()->getWorldTransform(),
                        bone_transforms,
                        m->getSegment(i).skin_data->bind_transforms
                    });*/
                }
            }
        }
        for(auto l : omnis) {
            dl.omnis.emplace_back(DrawList::OmniLight{l->getOwner()->getTransform()->getWorldPosition(), l->color, l->intensity, l->radius});
        }
    }

    Camera* getDefaultCamera() {
        return default_camera;
    }
    void setDefaultCamera(Camera* c) {
        default_camera = c;
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
        // TODO
        //default_camera = scene->getRoot()->find<Camera>(r.readStr());
    }
private:
    std::set<Model*> models;
    std::set<OmniLight*> omnis;
    Camera* default_camera = 0;
    GameScene* scene = 0;
};
STATIC_RUN(RenderController) {
    rttr::registration::class_<RenderController>("RenderController")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
