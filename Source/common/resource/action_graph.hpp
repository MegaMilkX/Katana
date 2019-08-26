#ifndef ACTION_GRAPH_HPP
#define ACTION_GRAPH_HPP

#include "resource.h"
#include "../gfxm.hpp"

#include "../resource/animation.hpp"
#include "../resource/skeleton.hpp"

#include "../util/make_unique_string.hpp"

class ActionGraph;

class ActionGraphParams {
    struct Param {
        std::string name;
        float value;
    };
    std::vector<Param> params;

public:
    void resize(size_t sz) {
        params.resize(sz);
    }
    size_t paramCount() const {
        return params.size();
    }
    size_t createParam(const std::string& name) {
        std::set<std::string> names;
        for(auto& p : params) {
            names.insert(p.name);
        }
        std::string name_ = makeUniqueString(names, name);
        
        size_t i = params.size();
        params.emplace_back(Param{name_, .0f});
        return i;
    }
    void renameParam(size_t i, const std::string& new_name) {
        if(i >= params.size()) {
            return;
        }
        
        std::set<std::string> names;
        for(auto& p : params) {
            names.insert(p.name);
        }
        std::string name_ = makeUniqueString(names, new_name);

        params[i].name = name_;
    }
    Param& getParam(size_t i) {
        return params[i];
    }
};

class ActionGraphNode;
struct ActionGraphTransition {
    enum CONDITION {
        LARGER,
        LARGER_EQUAL,
        LESS,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL
    };
    struct Condition {
        size_t param;
        CONDITION type;
        float ref_value;
    };

    float blendTime = 0.1f;
    ActionGraphNode* from = 0;
    ActionGraphNode* to = 0;
    std::vector<Condition> conditions;
};

class ActionGraphNode {
    std::string name = "Action";
    gfxm::vec2 editor_pos;
    std::set<ActionGraphTransition*> out_transitions;
public:
    std::shared_ptr<Animation> anim;
    float cursor = .0f;

    const std::string& getName() const { return name; }
    void               setName(const std::string& value) { name = value; }
    const gfxm::vec2&  getEditorPos() const { return editor_pos; }
    void               setEditorPos(const gfxm::vec2& value) { editor_pos = value; }

    std::set<ActionGraphTransition*>& getTransitions() {
        return out_transitions;
    }

    void               update(
        float dt, 
        std::vector<AnimSample>& samples, 
        const std::map<Animation*, std::vector<int32_t>>& mappings,
        float weight
    );

    void               makeMappings(Skeleton* skel, std::map<Animation*, std::vector<int32_t>>& mappings);

    void               write(out_stream& out);
    void               read(in_stream& in);
};

class ActionGraphLayer {
public:
};

// ==========================

class ActionGraph : public Resource {
    RTTR_ENABLE(Resource)

    std::vector<ActionGraphTransition*> transitions;
    std::vector<ActionGraphNode*> actions;
    size_t entry_action = 0;
    size_t current_action = 0;
    float trans_weight = 1.0f;
    float trans_speed = 0.1f;
    std::vector<AnimSample> trans_samples;
    ActionGraphParams param_table;

    void pickEntryAction();

public:
    const char* getWriteExtension() const override { return "action_graph"; }

    ActionGraphNode* createAction(const std::string& name = "Action");
    void             renameAction(ActionGraphNode* action, const std::string& name);
    ActionGraphTransition* createTransition(const std::string& from, const std::string& to);

    ActionGraphNode* findAction(const std::string& name);
    int32_t          findActionId(const std::string& name);

    void            deleteAction(ActionGraphNode* action);
    void            deleteTransition(ActionGraphTransition* transition);

    const std::vector<ActionGraphNode*>&       getActions() const;
    const std::vector<ActionGraphTransition*>& getTransitions() const;

    ActionGraphNode*                           getAction(size_t i);

    size_t                                     getEntryActionId();
    ActionGraphNode*                           getEntryAction();
    void                                       setEntryAction(const std::string& name);

    ActionGraphParams&                         getParams();

    void                                       update(
        float dt, 
        std::vector<AnimSample>& samples, 
        const std::map<Animation*, std::vector<int32_t>>& mappings
    );

    void               makeMappings(Skeleton* skel, std::map<Animation*, std::vector<int32_t>>& mappings);

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
