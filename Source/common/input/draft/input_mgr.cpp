#include "input_mgr.hpp"


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

InputListenerStack& InputMgr2::getListenerStack() {
    return listener_stack;
}

void InputMgr2::update() {
    for(auto& d : devices) {
        d->update();
    }

    for(size_t i = 0; i < actions.size(); ++i) {
        auto& action = actions[i];
        input_action_uid_t action_uid = i + 1;

        for(auto& input : action.inputs) {
            InputAdapter* adp = users[0].getAdapter(input.adapter_type); // TODO: Template getAdapter creates adapter if it doesnt exist, but this version doesnt
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

            if(!input.is_pressed && input_satisfied) { // ON PRESS
                input.is_pressed = true;
                for(size_t j = 0; j < listener_stack.stackLength(); ++j) {
                    auto listener_uid = listener_stack.getStackElement(j);
                    auto listener = listener_stack.deref(listener_uid);
                    bool event_consumed = false;
                    for(auto& act : listener->expected_actions) {
                        if(act.action_uid == action_uid && act.type == INPUT_ACTION_PRESS) {
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
                for(size_t j = 0; j < listener_stack.stackLength(); ++j) {
                    auto listener_uid = listener_stack.getStackElement(j);
                    auto listener = listener_stack.deref(listener_uid);
                    bool event_consumed = false;
                    for(auto& act : listener->expected_actions) {
                        if(act.action_uid == action_uid && act.type == INPUT_ACTION_RELEASE) {
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