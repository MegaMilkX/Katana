#ifndef DOC_ACTION_GRAPH_HPP
#define DOC_ACTION_GRAPH_HPP

#include "editor_document.hpp"
#include "../common/resource/action_graph.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../common/attributes/light_source.hpp"
#include "../common/attributes/skeleton_ref.hpp"

class DocActionGraph : public EditorDocumentTyped<ActionGraph> {
    // Preview stuff
    GuiViewport viewport;
    GameScene scn;

    ktNode* cam_pivot = 0;
    DirLight* cam_light = 0;
    // ========

    bool first_use = true;
    ActionGraphNode* selected_action = 0;
    ActionGraphTransition* selected_transition = 0;

    AnimBlackboard blackboard;

    std::vector<AnimSample> sample_buffer;

    void setReferenceObject(ktNode* node);

public:
    DocActionGraph();

    virtual void onGui(Editor* ed, float dt);
    void onGuiToolbox(Editor* ed) override;

    void onResourceSet() override;
};
STATIC_RUN(DocActionGraph) {
    regEditorDocument<DocActionGraph>({ "action_graph" });
}

#endif
