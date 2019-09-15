#ifndef DOC_ACTION_GRAPH_HPP
#define DOC_ACTION_GRAPH_HPP

#include "editor_document.hpp"
#include "../common/resource/action_graph.hpp"
#include "../common/resource/resource_tree.hpp"

class DocActionGraph : public EditorDocumentTyped<ActionGraph> {
    bool first_use = true;
    ActionGraphNode* selected_action = 0;
    ActionGraphTransition* selected_transition = 0;
public:
    DocActionGraph();

    virtual void onGui(Editor* ed, float dt);
    void onGuiToolbox(Editor* ed) override;
};
STATIC_RUN(DocActionGraph) {
    regEditorDocument<DocActionGraph>({ "action_graph" });
}

#endif
