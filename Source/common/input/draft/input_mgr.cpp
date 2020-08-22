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

void InputMgr2::updateUserActionBuffers() {
    for(auto& u : users) {
        u.resizeStateBuffers(actions.size(), axes.size());
    }
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
    updateUserActionBuffers();
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

void InputMgr2::setAction(const char* name, const InputAction2& action) {
    auto it = action_uids.find(name);
    if(it == action_uids.end()) {
        input_action_uid_t uid = actions.size() + 1;
        actions.push_back(action);
        actions.back().name = name;
        action_uids[name] = uid;

        updateUserActionBuffers();
    } else {
        actions[it->second] = action;
        actions[it->second].name = name;
    }
}

input_action_uid_t InputMgr2::getActionUid(const char* name) {
    auto it = action_uids.find(name);
    if(it == action_uids.end()) {
        return 0;
    }
    return it->second;
}

void InputMgr2::setAxis(const char* name, const InputAxis& axis) {
    auto it = axis_uids.find(name);
    if(it == axis_uids.end()) {
        input_axis_uid_t uid = axes.size() + 1;
        axes.push_back(axis);
        axes.back().name = name;
        axis_uids[name] = uid;

        updateUserActionBuffers();
    } else {
        axes[it->second] = axis;
        axes[it->second].name = name;
    }
}

input_axis_uid_t InputMgr2::getAxisUid(const char* name) {
    auto it = axis_uids.find(name);
    if(it == axis_uids.end()) {
        return 0;
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

void InputMgr2::postMessage(const InputActionEvent& msg) {
    deferred_events.push(msg);
}
void InputMgr2::postMessage(const InputAxisEvent& msg) {
    deferred_axis_events.push(msg);
}

void InputMgr2::update(float dt) {
    for(auto& u : users) {
        u.clearAdapterKeyTables();
    }

    for(auto& d : devices) {
        d->update();
    }

    for(size_t i = 0; i < axes.size(); ++i) {
        auto& axis = axes[i];
        input_axis_uid_t axis_uid = i + 1;

        for(int user_id = 0; user_id < users.size(); ++user_id) {
            InputUserLocalAxisState& lcl_axis_state = users[user_id].getLocalAxisState(axis_uid);
            float axis_sum = .0f;
            bool axis_input_changed = false;
            float prev_axis_sum = lcl_axis_state.value;

            for(auto& input : axis.keys) {
                InputAdapter* adp = users[user_id].getAdapter(input.adapter_type);
                if(!adp) {
                    continue;
                }

                float key_value = adp->getKeyState(input.key);
                axis_sum += key_value * input.multiplier;
            }

            if(prev_axis_sum != axis_sum) {
                lcl_axis_state.value = axis_sum;

                InputAxisEvent evt;
                evt.axis = axis_uid;
                evt.user_id = user_id;
                evt.value = axis_sum;
                postMessage(evt);                
            }
        }
    }

    for(size_t i = 0; i < actions.size(); ++i) {
        auto& action = actions[i];
        input_action_uid_t action_uid = i + 1;

        for(int user_id = 0; user_id < users.size(); ++user_id) {
            InputUserLocalActionState& user_lcl_action_state = users[user_id].getLocalActionState(action_uid);

            for(auto& input : action.inputs) {
                InputAdapter* adp = users[user_id].getAdapter(input.adapter_type); // TODO: Template getAdapter creates adapter if it doesnt exist, but this overload doesnt
                if(!adp) {
                    continue;
                }

                bool input_satisfied = false;
                for(int j = 0; j < INPUT_MAX_ACTION_KEY_COMBINATION; ++j) {
                    if(input.keys[j] < 0) {
                        break;
                    } 
                    if(adp->getKeyState(input.keys[j]) != .0f) {
                        input_satisfied = true;
                    } else {
                        input_satisfied = false;
                        break;
                    }
                }

                if(user_lcl_action_state.is_pressed && input_satisfied) {
                    float prev_pressed_time = user_lcl_action_state.press_time;
                    user_lcl_action_state.press_time += dt;
                    float cur_pressed_time = user_lcl_action_state.press_time;

                    //deferred_events.push(InputActionEvent{ INPUT_ACTION_HOLD, action_uid, user_id });

                    cur_pressed_time -= INPUT_REPEAT_DELAY;
                    if(cur_pressed_time > .0f) {
                        prev_pressed_time -= INPUT_REPEAT_DELAY;

                        int prev_repeat_count = prev_pressed_time / INPUT_REPEAT_RATE;
                        int cur_repeat_count = cur_pressed_time / INPUT_REPEAT_RATE;

                        if(cur_repeat_count - prev_repeat_count > 0) {
                            postMessage(InputActionEvent{ INPUT_ACTION_PRESS_REPEAT, action_uid, user_id });
                        }
                    }
                }

                if(!user_lcl_action_state.is_pressed && input_satisfied) { // ON PRESS
                    user_lcl_action_state.is_pressed = true;
                    user_lcl_action_state.press_time = .0f;
                    postMessage(InputActionEvent{ INPUT_ACTION_PRESS, action_uid, user_id });

                } else if(user_lcl_action_state.is_pressed && !input_satisfied) { // ON RELEASE
                    postMessage(InputActionEvent{ INPUT_ACTION_RELEASE, action_uid, user_id });
                    if(user_lcl_action_state.press_time <= INPUT_TAP_TIME) {
                        postMessage(InputActionEvent{ INPUT_ACTION_TAP, action_uid, user_id });
                    }

                    user_lcl_action_state.is_pressed = false;
                    user_lcl_action_state.press_time = .0f;
                }
            }
        }
    }

    while(!deferred_axis_events.empty()) {
        auto evt = deferred_axis_events.front();
        deferred_axis_events.pop();

        for(int j = listener_stack.size() - 1; j >= 0; --j) {
            bool event_consumed = listener_stack[j]->onAxis(evt);
            if(event_consumed) {
                break;
            }
        }
    }
    while(!deferred_events.empty()) {
        auto evt = deferred_events.front();
        deferred_events.pop();

        for(int j = listener_stack.size() - 1; j >= 0; --j) {
            bool event_consumed = listener_stack[j]->onAction(evt);
            if(event_consumed) {
                break;
            }
        }
    }
}