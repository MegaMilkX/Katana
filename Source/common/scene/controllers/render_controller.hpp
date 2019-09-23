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

#include "../../util/octree.hpp"

class RenderController : public SceneEventFilter<RenderableBase, Camera, OmniLight, DirLight> {
    RTTR_ENABLE(SceneController)

    std::set<RenderableBase*> renderables;
    std::list<RenderableBase*> moved_renderables;
    std::unordered_map<RenderableBase*, int32_t> renderable_to_octree;
    std::set<OmniLight*> omnis;
    std::set<DirLight*> dir_lights;
    Camera* default_camera = 0;
    GameScene* scene = 0;

    Octree octree;

public:
    virtual void init(GameScene* s) {
        scene = s;
    }
    virtual void cleanup() {
        moved_renderables.clear();
    }

    virtual void onAttribCreated(RenderableBase* m) { 
        renderables.insert(m);
        gfxm::aabb box;
        m->getOwner()->makeAabb(box);
        int32_t ocid = octree.createObject(box, (void*)m);
        renderable_to_octree[m] = ocid;
    }
    virtual void onAttribRemoved(RenderableBase* m) { 
        renderables.erase(m);
        int32_t ocid = renderable_to_octree[m];
        renderable_to_octree.erase(m);
        octree.deleteObject(ocid);
    } 
    virtual void onAttribCreated(OmniLight* l) { omnis.insert(l); }
    virtual void onAttribRemoved(OmniLight* l) { omnis.erase(l); }
    virtual void onAttribCreated(DirLight* l) { dir_lights.insert(l); }
    virtual void onAttribRemoved(DirLight* l) { dir_lights.erase(l); }
    virtual void onAttribCreated(Camera* c) { if(!default_camera) default_camera = c; }
    virtual void onAttribRemoved(Camera* c) { if(c == default_camera) default_camera = 0; }

    void onAttribTransformed(RenderableBase* r) override {
        moved_renderables.insert(moved_renderables.end(), r);
    }

    void getDrawList(DrawList& dl) {
        for(auto r : renderables) {
            r->addToDrawList(dl);
        }
        for(auto l : omnis) {
            dl.omnis.emplace_back(DrawList::OmniLight{l->getOwner()->getTransform()->getWorldPosition(), l->color, l->intensity, l->radius});
        }
        for(auto l : dir_lights) {
            dl.dir_lights.emplace_back(
                DrawList::DirLight{
                    l->getOwner()->getTransform()->back(),
                    l->color,
                    l->intensity
                }
            );
        }
    }

    void getDrawList(DrawList& dl, const gfxm::frustum& frust) {
        for(auto r : moved_renderables) {
            gfxm::aabb box;
            r->getOwner()->makeAabb(box);
            octree.updateObject(renderable_to_octree[r], box);
        }
        moved_renderables.clear();

        auto cell_list = octree.listVisibleCells(frust);
        for(auto c : cell_list) { 
            for(auto o : octree.getCell(c)->objects) {
                RenderableBase* r = (RenderableBase*)octree.getObject(o)->user_ptr;
                if(gfxm::frustum_vs_aabb(frust, octree.getObject(o)->aabb)) {
                    r->addToDrawList(dl);
                }
            }
        }

        for(auto l : omnis) {
            dl.omnis.emplace_back(DrawList::OmniLight{l->getOwner()->getTransform()->getWorldPosition(), l->color, l->intensity, l->radius});
        }
        for(auto l : dir_lights) {
            dl.dir_lights.emplace_back(
                DrawList::DirLight{
                    l->getOwner()->getTransform()->back(),
                    l->color,
                    l->intensity
                }
            );
        }
    }

    Camera* getDefaultCamera() {
        return default_camera;
    }
    void setDefaultCamera(Camera* c) {
        default_camera = c;
    }

    void debugDraw(DebugDraw& dd) {
        octree.debugDraw(dd);
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

};
STATIC_RUN(RenderController) {
    rttr::registration::class_<RenderController>("RenderController")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
