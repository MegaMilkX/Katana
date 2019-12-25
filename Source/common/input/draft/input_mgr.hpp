#ifndef INPUT_MGR2_HPP
#define INPUT_MGR2_HPP


#include <assert.h>
#include <vector>
#include <set>
#include <stack>
#include <list>
#include <memory>
#include "input_user.hpp"
#include "input_device.hpp"
#include "input_action.hpp"
#include "input_listener.hpp"


typedef size_t input_listener_uid_t;

class InputListenerStack {
    std::vector<input_listener_uid_t> stack;
    std::vector<InputActionListener> listener_pool;
    std::set<input_listener_uid_t> free_slots;

    void removeFromStack(input_listener_uid_t uid) {
        for(size_t i = 0; i < stack.size(); ++i) {
            if(stack[i] == uid) {
                stack.erase(stack.begin() + i);
                break;
            }
        }
    }

public:
    input_listener_uid_t acquire() {
        input_listener_uid_t uid = 0;
        if(!free_slots.empty()) {
            uid = *free_slots.begin();
            listener_pool[uid] = InputActionListener();
            free_slots.erase(free_slots.begin());
        } else {
            uid = listener_pool.size();
            listener_pool.push_back(InputActionListener());
        }
        stack.push_back(uid);
        return uid;
    }
    void                 free(input_listener_uid_t uid) {
        removeFromStack(uid);
        free_slots.insert(uid);
    }
    InputActionListener* deref(input_listener_uid_t uid) {
        return &listener_pool[uid];
    }

    void moveToTop(input_listener_uid_t uid) {
        removeFromStack(uid);
        stack.push_back(uid);
    }

    size_t stackLength() const {
        return stack.size();
    }
    input_listener_uid_t getStackElement(size_t id) {
        return stack[stack.size() - 1 - id];
    }


};

class InputMgr2 {
    std::vector<InputUser>                  users;
    std::set<std::shared_ptr<InputDevice>>  devices;
    std::map<std::string, InputAction>      actions;
    InputListenerStack                      listener_stack;
    std::queue<InputActionDesc>             deferred_callbacks;
public:
    InputMgr2() {
        setUserCount(1);
    }
    size_t userCount() const { 
        return users.size(); 
    }
    void setUserCount(size_t count) {
        assert(count > 0);
        users.resize(count);
    }
    InputUser* getUser(size_t idx) { 
        assert(idx < users.size());
        return &users[idx]; 
    }

    // Ownership is transferred to input manager
    void addDevice(InputDevice* device) {
        devices.insert(std::shared_ptr<InputDevice>(device));
        assignDevice(device, 0);
    }
    void clearDevices() {
        devices.clear();
    }
    void assignDevice(InputDevice* device, size_t user_idx) {
        assert(device && user_idx < users.size());
        device->setUser(&users[user_idx]);
    }

    void addAction(const char* name, const InputAction& action) {
        actions[name] = action;
    }

    InputListenerStack& getListenerStack() {
        return listener_stack;
    }

    void update() {
        for(auto& d : devices) {
            d->update();
        }

        for(auto& kv : actions) {
            InputAction& action = kv.second;
            for(auto& input : action.inputs) {
                InputAdapter* adp = users[0].getAdapter(input.adapter_type); // TODO: Template getAdapter creates adapter if it doesnt exist, but this version doesnt
                if(!adp) {
                    continue;
                }

                bool input_satisfied = false;
                for(int i = 0; i < 3 /* SHORTCUT MAX KEY NUM */; ++i) {
                    if(input.keys[i] < 0) {
                        break;
                    } 
                    if(adp->getKeyState(input.keys[i])) {
                        input_satisfied = true;
                    } else {
                        input_satisfied = false;
                        break;
                    }
                }

                if(!input.is_pressed && input_satisfied) { // ON PRESS
                    input.is_pressed = true;
                    for(size_t i = 0; i < listener_stack.stackLength(); ++i) {
                        auto listener_uid = listener_stack.getStackElement(i);
                        auto listener = listener_stack.deref(listener_uid);
                        bool event_consumed = false;
                        for(auto& act : listener->expected_actions) {
                            if(act.name == kv.first && act.type == INPUT_ACTION_PRESS) {
                                event_consumed = true;
                                deferred_callbacks.push(act);
                                break;
                            }
                        }
                        if(event_consumed) {
                            break;
                        }
                    }
                } else if(input.is_pressed && !input_satisfied) { // ON RELEASE
                    input.is_pressed = false;
                    for(size_t i = 0; i < listener_stack.stackLength(); ++i) {
                        auto listener_uid = listener_stack.getStackElement(i);
                        auto listener = listener_stack.deref(listener_uid);
                        bool event_consumed = false;
                        for(auto& act : listener->expected_actions) {
                            if(act.name == kv.first && act.type == INPUT_ACTION_RELEASE) {
                                event_consumed = true;
                                deferred_callbacks.push(act);
                                break;
                            }
                        }
                        if(event_consumed) {
                            break;
                        }
                    }
                }
            }
        }

        while(!deferred_callbacks.empty()) {
            auto cb = deferred_callbacks.front();
            deferred_callbacks.pop();
            cb.callback();
        }
    }
};


inline InputMgr2& getInputMgr() {
    static InputMgr2 mgr;
    return mgr;
}


class InputListenerHdl {
    input_listener_uid_t uid;
public:
    InputListenerHdl() {
        uid = getInputMgr().getListenerStack().acquire();
    }
    virtual ~InputListenerHdl() {
        getInputMgr().getListenerStack().free(uid);
    }

    void moveToTop() {
        getInputMgr().getListenerStack().moveToTop(uid);
    }

    void bindPress(const char* name, std::function<void(void)> cb) {
        getInputMgr().getListenerStack().deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_PRESS, name, cb });
    }
    void bindRelease(const char* name, std::function<void(void)> cb) {
        getInputMgr().getListenerStack().deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_RELEASE, name, cb });
    }
    void bindHold(const char* name, float time, std::function<void(void)> cb) {
        getInputMgr().getListenerStack().deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_HOLD, name, cb });
    }
    void bindTap(const char* name, std::function<void(void)> cb) {
        getInputMgr().getListenerStack().deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_TAP, name, cb });
    }
};


#endif
