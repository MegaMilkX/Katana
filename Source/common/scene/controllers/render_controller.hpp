#ifndef RENDER_CONTROLLER_HPP
#define RENDER_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../attributes/model.hpp"
#include "../../attributes/camera.hpp"
#include "../../attributes/light_source.hpp"

#include "../../../common/draw_list.hpp"

#include <set>

#include "../../../common/util/imgui_helpers.hpp"

class RenderController : public SceneEventFilter<RenderableBase, Camera, OmniLight> {
    RTTR_ENABLE(SceneController)
public:
    virtual void init(GameScene* s) {
        scene = s;
    }

    virtual void onAttribCreated(RenderableBase* m) { renderables.insert(m); }
    virtual void onAttribRemoved(RenderableBase* m) { renderables.erase(m); } 
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
        for(auto r : renderables) {
            r->addToDrawList(dl);
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
    std::set<RenderableBase*> renderables;
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
