#ifndef INPUT_MGR_HPP
#define INPUT_MGR_HPP

#include <memory>
#include <set>

#include "input_table.hpp"
#include "input_listener.hpp"

#include "../util/log.hpp"

typedef size_t input_key_hdl_t;

class InputMgr;
class ScopedInputListener {
public:
    ScopedInputListener() {}
    ScopedInputListener(InputMgr*);
    ScopedInputListener(ScopedInputListener& other) {
        mgr = other.mgr;
        lis = other.lis;
        ref_count = other.ref_count;
        if(ref_count) *ref_count++;
    }
    ~ScopedInputListener();
    void bindAxis(const std::string& axis, InputListener::axis_cb_t cb) {
        if(!lis) return;
        lis->bindAxis(axis, cb);
    }
    void bindActionPress(const std::string& action, InputListener::action_cb_t cb) {
        if(!lis) return;
        lis->bindActionPress(action, cb);
    }
    void bindActionRelease(const std::string& action, InputListener::action_cb_t cb) {
        if(!lis) return;
        lis->bindActionRelease(action, cb);
    }
private:
    InputMgr* mgr = 0;
    InputListener* lis = 0;
    int* ref_count = 0;
};

class InputMgr {
public:
    InputTable& getTable() { return input_table; }

    ScopedInputListener createScopedListener() {
        return ScopedInputListener(this);
    }
    InputListener* createListener() {
        InputListener* listener = new InputListener();
        listeners.insert(listener);
        return listener;
    }
    void removeListener(InputListener* l) {
        delete l;
        listeners.erase(l);
    }

    input_key_hdl_t getKeyHandle(const std::string& key) {
        if(key_to_handle.count(key)) {
            return key_to_handle[key];
        }
        key_to_handle[key] = key_values.size();
        key_values.emplace_back(0.0f);
        return key_to_handle[key];
    }

    void set(const std::string& key, float value, bool optimize = true) {
        //set(getKeyHandle(key), value);
        const float press_threshold = 0.6f;

        float& key_value = key_values[getKeyHandle(key)];
        float prev_value = key_value;
        key_value = value;

        if(prev_value < press_threshold && value >= press_threshold) {
            for(auto& l : listeners) {
                for(auto& a : input_table.getActions(key)) {
                    l->triggerPress(a);
                }
            }
        } else if(prev_value >= press_threshold && value < press_threshold) {
            for(auto& l : listeners) {
                for(auto& a : input_table.getActions(key)) {
                    l->triggerRelease(a);
                }
            }
        } else if(prev_value == value && optimize) {
            return;
        }

        for(auto& l : listeners) {
            for(auto& a : input_table.getAxes(key)) {
                l->triggerAxis(a.first, value * a.second);
            }
        }
    }
    void set(input_key_hdl_t key, float value) {
        
    }

    void update() {
        
    }
private:
    InputTable input_table;
    std::map<std::string, input_key_hdl_t> key_to_handle;
    std::vector<float> key_values;

    std::set<InputListener*> listeners;
};

inline ScopedInputListener::ScopedInputListener(InputMgr* mgr)
: mgr(mgr) {
    ref_count = new int(1);
    if(!mgr) return;
    lis = mgr->createListener();
}
inline ScopedInputListener::~ScopedInputListener() {
    if(ref_count) {
        *ref_count--;
        if(ref_count == 0) {
            delete ref_count;
        }
    }
    if(!mgr || !lis || !ref_count || *ref_count > 0) return;
    mgr->removeListener(lis);
}

inline InputMgr& input() {
    static InputMgr mgr;
    return mgr;
}

#endif
