#ifndef STATEFUL_ACTOR_HPP
#define STATEFUL_ACTOR_HPP

#include "actor_object.hpp"

class StatefulActor : public ActorObject {
    RTTR_ENABLE(ActorObject)
public:
    typedef std::function<void(void)> state_init_cb_t;
    typedef std::function<void(void)> state_update_cb_t;
    typedef std::function<bool(void)> trans_cond_fn_t;

    struct State;
    struct Transition {
        trans_cond_fn_t cond_func;
        std::string target;
    };
    struct State {
        state_init_cb_t init_func;
        state_update_cb_t update_func;
        std::vector<Transition> transitions;
    };

    void addState(
        const std::string& name,
        state_init_cb_t init_func,
        state_update_cb_t update_func
    ) {
        auto ptr = std::shared_ptr<State>(new State{init_func, update_func});
        states[name] = ptr;
    }
    void addTransition(
        const std::string& origin, 
        const std::string& target,
        trans_cond_fn_t func
    ) {
        auto& it = states.find(origin);
        if(it != states.end()) {
            it->second->transitions.emplace_back(Transition{func, target});
        }
    }
    void addTransitions(
        const std::vector<std::string>& origins,
        const std::string& target,
        trans_cond_fn_t func
    ) {
        for(auto& o : origins) {
            addTransition(o, target, func);
        }
    }

    void switchState(const std::string& target) {
        auto& it = states.find(target);
        if(it != states.end()) {
            it->second->init_func();
            current_state = it->second.get();
        } else {
            LOG("State " << target << " not found");
        }
    }

    virtual void update() {
        if(!current_state) return;
        for(auto& t : current_state->transitions) {
            if(t.cond_func()) {
                switchState(t.target);
            }
        }
        if(!current_state) return;
        current_state->update_func();
    }
private:
    std::map<std::string, std::shared_ptr<State>> states;
    State* current_state = 0;
};

#endif
