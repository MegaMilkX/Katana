#ifndef ACTION_GRAPH_HPP
#define ACTION_GRAPH_HPP

#include "resource.h"
#include "../gfxm.hpp"

class ActionGraphNode;
struct ActionGraphTransition {
    float blendTime = 0.1f;
    ActionGraphNode* from = 0;
    ActionGraphNode* to = 0;
};

class ActionGraphNode {
    std::string name = "Action";
    gfxm::vec2 editor_pos;
public:
    const std::string& getName() const { return name; }
    void               setName(const std::string& value) { name = value; }
    const gfxm::vec2&  getEditorPos() const { return editor_pos; }
    void               setEditorPos(const gfxm::vec2& value) { editor_pos = value; }
};

class ActionGraphLayer {
public:
};

// ==========================

class ActionGraph : public Resource {
    RTTR_ENABLE(Resource)

    std::vector<ActionGraphTransition*> transitions;
    std::vector<ActionGraphNode*> actions;
    ActionGraphNode* entry_action = 0;

    void pickEntryAction();

public:
    const char* getWriteExtension() const override { return "action_graph"; }

    ActionGraphNode* createAction(const std::string& name = "Action");
    void             renameAction(ActionGraphNode* action, const std::string& name);
    ActionGraphTransition* createTransition(const std::string& from, const std::string& to);

    ActionGraphNode* findAction(const std::string& name);

    void            deleteAction(ActionGraphNode* action);
    void            deleteTransition(ActionGraphTransition* transition);

    const std::vector<ActionGraphNode*>       getActions() const;
    const std::vector<ActionGraphTransition*> getTransitions() const;

    ActionGraphNode*                          getEntryAction();
    void                                      setEntryAction(const std::string& name);

    void serialize(out_stream& out) override;
    bool deserialize(in_stream& in, size_t sz) override;
};
STATIC_RUN(ActionGraph) {
    rttr::registration::class_<ActionGraph>("ActionGraph")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
