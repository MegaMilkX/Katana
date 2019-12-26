#include "input_mgr.hpp"

// === PRIVATE ===

int InputMgr2::findListener(InputListenerBase* lis) {
    int lis_stack_id = -1;
    for(int i = 0; i < listener_stack.size(); ++i) {
        if(listener_stack[i] == lis) {
            lis_stack_id = i;
            break;
        }
    }
    return lis_stack_id;
}

// === PUBLIC ===

InputMgr2::InputMgr2() {
    setUserCount(1);
}
size_t InputMgr2::userCount() const { 
    return users.size(); 
}
void InputMgr2::setUserCount(size_t count) {
    assert(count > 0);
    users.resize(count);
}
InputUser* InputMgr2::getUser(size_t idx) { 
    assert(idx < users.size());
    return &users[idx]; 
}

// Ownership is transferred to input manager
void InputMgr2::addDevice(InputDevice* device) {
    devices.insert(std::shared_ptr<InputDevice>(device));
    assignDevice(device, 0);
}
void InputMgr2::clearDevices() {
    devices.clear();
}
void InputMgr2::assignDevice(InputDevice* device, size_t user_idx) {
    assert(device && user_idx < users.size());
    device->setUser(&users[user_idx]);
}

void InputMgr2::setAction(const char* name, const InputAction& action) {
    auto it = action_uids.find(name);
    if(it == action_uids.end()) {
        input_action_uid_t uid = actions.size() + 1;
        actions.push_back(action);
        actions.back().name = name;
        action_uids[name] = uid;
    } else {
        actions[it->second] = action;
        actions[it->second].name = name;
    }
}

input_action_uid_t InputMgr2::getActionUid(const char* name) {
    auto it = action_uids.find(name);
    if(it == action_uids.end()) {
        return -1;
    }
    return it->second;
}

void InputMgr2::addListener(InputListenerBase* lis) {
    int lis_stack_id = findListener(lis);
    if(lis_stack_id < 0) {
        listener_stack.push_back(lis);
    }
}
void InputMgr2::removeListener(InputListenerBase* lis) {
    int lis_stack_id = findListener(lis);
    if(lis_stack_id >= 0) {
        listener_stack.erase(listener_stack.begin() + lis_stack_id);
    }
}
void InputMgr2::moveListenerToTop(InputListenerBase* lis) {
    int lis_stack_id = findListener(lis);
    if(lis_stack_id >= 0) {
        listener_stack.erase(listener_stack.begin() + lis_stack_id);
        listener_stack.push_back(lis);
    }
}

void InputMgr2::update(float dt) {
    for(auto& d : devices) {
        d->update();
    }

    for(size_t i = 0; i < actions.size(); ++i) {
        auto& action = actions[i];
        input_action_uid_t action_uid = i + 1;

        for(int user_id = 0; user_id < users.size(); ++user_id) {
            for(auto& input : action.inputs) {
                InputAdapter* adp = users[user_id].getAdapter(input.adapter_type); // TODO: Template getAdapter creates adapter if it doesnt exist, but this overload doesnt
                if(!adp) {
                    continue;
                }

                bool input_satisfied = false;
                for(int j = 0; j < 3 /* SHORTCUT MAX KEY NUM */; ++j) {
                    if(input.keys[j] < 0) {
                        break;
                    } 
                    if(adp->getKeyState(input.keys[j])) {
                        input_satisfied = true;
                    } else {
                        input_satisfied = false;
                        break;
                    }
                }

                if(input.is_pressed && input_satisfied) {
                    float prev_pressed_time = input.pressed_time;
                    input.pressed_time += dt;
                    float cur_pressed_time = input.pressed_time;

                    //deferred_events.push(InputActionEvent{ INPUT_ACTION_HOLD, action_uid, user_id });

                    cur_pressed_time -= INPUT_REPEAT_DELAY;
                    if(cur_pressed_time > .0f) {
                        prev_pressed_time -= INPUT_REPEAT_DELAY;

                        int prev_repeat_count = prev_pressed_time / INPUT_REPEAT_RATE;
                        int cur_repeat_count = cur_pressed_time / INPUT_REPEAT_RATE;

                        if(cur_repeat_count - prev_repeat_count > 0) {
                            deferred_events.push(InputActionEvent{ INPUT_ACTION_PRESS_REPEAT, action_uid, user_id });
                        }
                    }
                }

                if(!input.is_pressed && input_satisfied) { // ON PRESS
                    input.is_pressed = true;
                    input.pressed_time = .0f;
                    deferred_events.push(InputActionEvent{ INPUT_ACTION_PRESS, action_uid, user_id });

                } else if(input.is_pressed && !input_satisfied) { // ON RELEASE
                    deferred_events.push(InputActionEvent{ INPUT_ACTION_RELEASE, action_uid, user_id });
                    if(input.pressed_time <= INPUT_TAP_TIME) {
                        deferred_events.push(InputActionEvent{ INPUT_ACTION_TAP, action_uid, user_id });
                    }

                    input.is_pressed = false;
                    input.pressed_time = .0f;
                }
            }
        }
    }

    while(!deferred_events.empty()) {
        auto evt = deferred_events.front();
        deferred_events.pop();

        for(int j = listener_stack.size() - 1; j >= 0; --j) {
            bool event_consumed = listener_stack[j]->onAction(evt.type, evt.action, evt.user_id);
            if(event_consumed) {
                break;
            }
        }
    }
}