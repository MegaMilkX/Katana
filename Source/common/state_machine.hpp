#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <vector>
#include <map>
#include <memory>
#include <functional>

template<typename T>
class StateMachine;

template<typename T>
class State {
public:
    typedef std::function<bool(T*)> cond_fun_t;

    virtual ~State() {}

    virtual void start(T*) = 0;
    virtual void update(T*) = 0;

    State* getTransitionTarget(T* object) {
        for(auto kv : transitions) {
            if(kv.second(object)) {
                return kv.first;
            }
        }
        return 0;
    }

    void _setFsm(StateMachine<T>* fsm) {
        this->fsm = fsm;
    }
    void _addTransition(State<T>* to, cond_fun_t cond) {
        transitions[to] = cond;
    }
protected:
    StateMachine<T>* fsm;
    std::map<State*, cond_fun_t> transitions;
};

template<typename T>
class StateWrapper : public State<T> {
public:
    typedef std::function<void(T*)> cb_t;

    virtual void start(T* v) {
        if(start_cb) start_cb(v);
    }
    virtual void update(T* v) {
        if(update_cb) update_cb(v);
    }

    cb_t start_cb;
    cb_t update_cb;
};

template<typename T>
class StateMachine {
public:
    typedef State<T> state_t;
    typedef StateWrapper<T> state_wrap_t;

    StateMachine(T* object)
    : object(object) {}

    state_t* createState(
        const std::string& name,
        typename StateWrapper<T>::cb_t start_cb,
        typename StateWrapper<T>::cb_t update_cb
    ) {
        state_wrap_t* state = 0;
        auto it = name_to_state.find(name);
        if(it == name_to_state.end()) {
            state = new state_wrap_t();
            name_to_state[name] = state;
            states.emplace_back(state);
        } else {
            state = it->second;
        }

        state->start_cb = start_cb;
        state->update_cb = update_cb;

        if(states.size() == 1) {
            switchTo(states.back());
        }

        return state;
    }

    void addTransition(const std::string& from, const std::string& to, typename state_t::cond_fun_t condition) {
        auto it = name_to_state.find(from);
        if(it == name_to_state.end()) {
            LOG_WARN("FROM state '" << from << "' doesn't exist");
        }
        state_wrap_t* a = it->second;
        it = name_to_state.find(to);
        if(it == name_to_state.end()) {
            LOG_WARN("TO state '" << to << "' doesn't exist");
        }
        state_wrap_t* b = it->second;

        a->_addTransition(b, condition);
    }

    void update() {
        if(!current_state) return;
        state_t* tgt = current_state->getTransitionTarget(object);
        if(tgt) switchTo(tgt);

        current_state->update(object);
    }
    void switchTo(state_t* s) {
        current_state = s;
        s->start(object);
    }
    void switchTo(const std::string& name) {
        auto it = name_to_state.find(name);
        if(it == name_to_state.end()) {
            return;
        }
        switchTo(it->second);
    }
private:
    T* object;
    state_t* current_state;
    std::vector<state_t*> states;
    std::map<std::string, state_wrap_t*> name_to_state;
};

#endif
