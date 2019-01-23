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
class StateMachine {
public:
    StateMachine(T* object)
    : object(object) {}

    // state pointer is now owned by the state machine
    void addState(State<T>* s) {
        s->_setFsm(this);
        states.emplace_back(s);
        if(states.size() == 1) {
            switchTo(states.back());
        }
    }
    void addTransition(State<T>* from, State<T>* to, typename State<T>::cond_fun_t condition) {
        from->_addTransition(to, condition);
    }
    void update() {
        if(!current_state) return;
        State<T>* tgt = current_state->getTransitionTarget(object);
        if(tgt) switchTo(tgt);

        current_state->update(object);
    }
    void switchTo(State<T>* s) {
        current_state = s;
        s->start(object);
    }
private:
    T* object;
    State<T>* current_state;
    std::vector<State<T>*> states;
};

#endif
