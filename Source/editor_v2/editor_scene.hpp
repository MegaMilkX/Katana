#ifndef EDITOR_SCENE_HPP
#define EDITOR_SCENE_HPP

#include "scene/game_scene.hpp"

#include "editor_component_desc.hpp"
#include "editor_object_desc.hpp"

class EditorScene : public SceneListener {
public:
    EditorScene(GameScene* scene)
    : scene(scene) {
        scene->getEventMgr().subscribe(this, EVT_OBJECT_CREATED);
        scene->getEventMgr().subscribe(this, EVT_OBJECT_REMOVED);
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_CREATED);
        scene->getEventMgr().subscribe(this, EVT_COMPONENT_REMOVED);
    }
    ~EditorScene() {
        scene->getEventMgr().unsubscribeAll(this);
    }

    IEditorObjectDesc* getObjectDesc(GameObject* o) {
        return object_desc_map[o].get();
    }
    IEditorComponentDesc* getComponentDesc(ObjectComponent* c) {
        return comp_desc_map[c].get();
    }

    void objectGui(GameObject* o) {
        object_desc_map[o]->gui();
    }
    void componentGui(ObjectComponent* c) {
        comp_desc_map[c]->gui();
    }
private:
    void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) {
        switch(e) {
        case EVT_OBJECT_CREATED:
            object_desc_map[sender].reset(sender->_newEditorObjectDesc());
            break;
        case EVT_OBJECT_REMOVED:
            object_desc_map.erase(sender);
            break;
        case EVT_COMPONENT_CREATED: {
            ObjectComponent* c = sender->get(payload.get_value<rttr::type>()).get();
            comp_desc_map[c].reset(c->_newEditorDescriptor());
        }
            break;
        case EVT_COMPONENT_REMOVED:
            comp_desc_map.erase(sender->get(payload.get_value<rttr::type>()).get());
            break;
        };
    }

    GameScene* scene = 0;
    std::map<GameObject*, std::shared_ptr<IEditorObjectDesc>> object_desc_map;
    std::map<ObjectComponent*, std::shared_ptr<IEditorComponentDesc>> comp_desc_map;
};

#endif
