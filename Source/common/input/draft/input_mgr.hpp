#ifndef INPUT_MGR2_HPP
#define INPUT_MGR2_HPP


#include <assert.h>
#include <vector>
#include <set>
#include <stack>
#include <memory>
#include "input_user.hpp"
#include "input_device.hpp"
#include "input_action.hpp"
#include "input_listener.hpp"

class InputMgr2 {
    std::vector<InputUser>                  users;
    std::set<std::shared_ptr<InputDevice>>  devices;
    std::map<std::string, InputAction>      actions;
    std::stack<InputActionListener>         listener_stack;
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

                if(!input.is_pressed && input_satisfied) {
                    input.is_pressed = true;
                    LOG(kv.first << " pressed");
                    // TODO: On press
                } else if(input.is_pressed && !input_satisfied) {
                    input.is_pressed = false;
                    LOG(kv.first << " released");
                    // TODO: On release
                }
            }
        }
    }
};


inline InputMgr2& getInputMgr() {
    static InputMgr2 mgr;
    return mgr;
}


#endif
