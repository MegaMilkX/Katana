#ifndef ANIMATION_GRAPH_HPP
#define ANIMATION_GRAPH_HPP


#include <vector>
#include "../util/anim_blackboard.hpp"


enum ANIM_FSM_CONDITION {
    ANIM_FSM_LARGER,
    ANIM_FSM_LARGER_EQUAL,
    ANIM_FSM_LESS,
    ANIM_FSM_LESS_EQUAL,
    ANIM_FSM_EQUAL,
    ANIM_FSM_NOT_EQUAL
};

struct ActionFSMCondition {
    size_t param_hdl;
    ANIM_FSM_CONDITION type;
    float ref_val;
};

inline bool checkCondition(ANIM_FSM_CONDITION type, float val, float ref_val) {
    bool res = false;
    switch(type) {
    case ANIM_FSM_LARGER: res = val > ref_val; break;
    case ANIM_FSM_LARGER_EQUAL: res = val >= ref_val; break;
    case ANIM_FSM_LESS: res = val < ref_val; break;
    case ANIM_FSM_LESS_EQUAL: res = val <= ref_val; break;
    case ANIM_FSM_EQUAL: res = val == ref_val; break;
    case ANIM_FSM_NOT_EQUAL: res = val != ref_val; break;
    };
    return res;
}

struct AnimStateConnection {
    size_t from = 0;
    size_t to = 0;
    float blendTime = 0.1f;
    std::vector<ActionFSMCondition> conditions;
};

struct AnimState {
    std::shared_ptr<BaseMotion> motion;
    std::vector<size_t> connections;
};

class AnimGraph {
    std::vector<AnimState> states;
    std::vector<AnimStateConnection> connections;
public:
    const AnimStateConnection& getConnection(size_t id) const {
        return connections[id];
    }
};


inline int pickTransition(const AnimGraph& graph, const AnimState& state) {
    for(size_t i = 0; i < state.connections.size(); ++i) {
        auto& conn = graph.getConnection(state.connections[i]);
        bool res = false;
        for(size_t j = 0; j < conn.conditions.size(); ++j) {
            res = checkCondition(conn.conditions[j].type, /* TODO */.0f, conn.conditions[j].ref_val);
            if(res == false) break;
        }
        return i;
    }
    return -1;
}


class AnimStateMachine {
    int current_state = -1;
    int current_transition = -1;
    float transition_cursor = .0f;
    std::shared_ptr<AnimGraph> graph;
    AnimBlackboard* blackboard = 0;

public:
    void setBlackboard(AnimBlackboard* bb) { blackboard = bb; }
    void setGraph(std::shared_ptr<AnimGraph> g) { graph = g; }

};


#endif
