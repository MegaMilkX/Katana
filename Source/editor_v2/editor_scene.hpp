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
    }
    ~EditorScene() {
        scene->getEventMgr().unsubscribeAll(this);
    }

    IEditorObjectDesc* getObjectDesc(GameObject* o) {
        return object_desc_map[o].get();
    }

    void objectGui(GameObject* o) {
        object_desc_map[o]->gui();
    }
private:
    virtual void onObjectCreated(GameObject* o) {
        object_desc_map[o].reset(o->_newEditorObjectDesc());
    }
    virtual void onObjectRemoved(GameObject* o) {
        object_desc_map.erase(o);
    }

    GameScene* scene = 0;
    std::map<GameObject*, std::shared_ptr<IEditorObjectDesc>> object_desc_map;
};

#endif
