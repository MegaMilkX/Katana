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

#include "../common/util/audio/audio_mixer.hpp"

#include "../common/application_state.hpp"

#include "scene_history.hpp"

enum TRANSFORM_GIZMO_MODE {
    TGIZMO_T,
    TGIZMO_R,
    TGIZMO_S
};

enum TRANSFORM_GIZMO_SPACE {
    TGIZMO_LOCAL,
    TGIZMO_WORLD
};

struct EditorState {
    TRANSFORM_GIZMO_MODE tgizmo_mode = TGIZMO_T;
    TRANSFORM_GIZMO_SPACE tgizmo_space = TGIZMO_LOCAL;

    bool debug_draw = true;
};

class Editor : public AppState {
public:
    Editor();
    ~Editor();
    virtual void onInit();
    virtual void onCleanup();
    virtual void onUpdate();
    virtual void onGui();

    GameScene* getScene();
    GameObject* getSelectedObject();
    EditorAssetInspector* getAssetInspector();
    void setSelectedObject(GameObject* o);

    void backupScene(const std::string& label = "");
    void redo();
    void undo();

    EditorState& getState();
private:
    bool showOpenSceneDialog();
    bool showSaveSceneDialog(GameScene* scene, bool forceDialog = false);

    EditorState editor_state;

    EditorViewport viewport;
    EditorSceneInspector scene_inspector;
    EditorDirView dir_view;
    EditorObjectInspector object_inspector;
    EditorAssetInspector asset_inspector;

    InputListener* input_lis = 0;

    std::shared_ptr<GameScene> scene;
    GameObject* selected_object = 0;
    SceneHistory history;

    std::string currentSceneFile;

    bool ctrl = false;
    bool shift = false;

    // windows
    bool history_open = false;
};

#endif
