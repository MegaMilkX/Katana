#ifndef DOC_ACTION_GRAPH_HPP
#define DOC_ACTION_GRAPH_HPP

#include "editor_document.hpp"
#include "../common/resource/action_graph.hpp"
#include "../common/resource/resource_tree.hpp"

class DocActionGraph : public EditorDocumentTyped<ActionGraph> {
    bool first_use = true;
    ActionGraphNode* selected_action = 0;
public:
    DocActionGraph();
    DocActionGraph(std::shared_ptr<ResourceNode>& node);

    virtual void onGui(Editor* ed);
};

#endif
