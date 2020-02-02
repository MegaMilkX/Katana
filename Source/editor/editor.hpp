#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "editor_scene_inspector.hpp"
#include "editor_object_inspector.hpp"
#include "editor_resource_tree.hpp"
#include "editor_document.hpp"

#include "../common/input/input_mgr.hpp"

#include "../common/util/filesystem.hpp"

#include "../common/scene/game_scene.hpp"

#include "../common/util/audio/audio_mixer.hpp"

#include "../common/application_state.hpp"

#include "scene_history.hpp"

#include "../common/util/object_set.hpp"

#include "../common/util/singleton.hpp"

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

class Editor : public AppState, public Singleton<Editor> {
public:
    Editor();
    ~Editor();
    virtual void onInit();
    virtual void onCleanup();
    virtual void onUpdate();
    virtual void onGui(float dt);

    EditorWindow* getFocusedWindow();
    EditorResourceTree& getResourceTree();
    void setCurrentDockspace(ImGuiID id);
    void setFocusedWindow(const std::string& name);
    void setFocusedWindow(EditorWindow* w);

    void addWindow(EditorWindow* doc);
    void tryOpenDocument(const std::string& res_path);
    void tryOpenNestedDocument(const std::string& res_path);
    void tryOpenDocumentFromPtr(std::shared_ptr<Resource> res);

    EditorState& getState();
private:
    int setupImguiLayout();

    bool showOpenSceneDialog();
    bool showSaveSceneDialog(GameScene* scene, bool forceDialog = false);

    EditorState editor_state;

    EditorResourceTree ed_resource_tree;
    std::set<EditorWindow*> open_documents;
    EditorWindow* focused_document = 0;

    ImGuiID dockspace_id;
    ImGuiID current_dockspace; // dockspace for the next opened document

    InputListener* input_lis = 0;

    SceneHistory history;

    std::string currentSceneFile;

    bool ctrl = false;
    bool shift = false;

    // windows
    bool history_open = false;
};

#endif
