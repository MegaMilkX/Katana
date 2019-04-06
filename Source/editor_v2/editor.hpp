#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "editor_viewport.hpp"
#include "editor_scene_inspector.hpp"
#include "editor_dir_view.hpp"
#include "editor_object_inspector.hpp"
#include "editor_asset_inspector.hpp"

#include "../common/input/input_mgr.hpp"

#include "../common/util/filesystem.hpp"

#include "scene/game_scene.hpp"

#include "editor_scene.hpp"

#include "editor_state.hpp"

#include "../common/util/audio/audio_mixer.hpp"

class Editor {
public:
    Editor();
    ~Editor();
    void init();
    void cleanup();
    void update(unsigned width, unsigned height, unsigned cursor_x, unsigned cursor_y);
    
    GameScene* getScene();
    EditorScene& getEditorScene();
    GameObject* getSelectedObject();
    EditorAssetInspector* getAssetInspector();
    void setSelectedObject(GameObject* o);
private:
    bool showOpenSceneDialog();
    bool showSaveSceneDialog(GameScene* scene, bool forceDialog = false);

    EditorViewport viewport;
    EditorSceneInspector scene_inspector;
    EditorDirView dir_view;
    EditorObjectInspector object_inspector;
    EditorAssetInspector asset_inspector;

    InputListener* input_lis = 0;

    std::shared_ptr<GameScene> scene;
    GameObject* selected_object = 0;

    std::shared_ptr<EditorScene> editor_scene;

    std::string currentSceneFile;
};

#endif
