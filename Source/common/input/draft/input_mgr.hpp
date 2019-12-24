#ifndef INPUT_MGR2_HPP
#define INPUT_MGR2_HPP


#include <assert.h>
#include <vector>
#include <set>
#include <memory>
#include "input_user.hpp"
#include "input_device.hpp"

class InputMgr2 {
    std::vector<InputUser> users;
    std::set<std::shared_ptr<InputDevice>> devices;
public:
    InputMgr2() {
        setUserCount(1);
    }
    size_t userCount() const { 
        return users.size(); 
    }
    void setUserCount(size_t count) {
        users.resize(count);
    }
    InputUser* getUser(size_t idx) { 
        assert(idx < users.size());
        return &users[idx]; 
    }

    // Ownership is transferred to input manager
    void addDevice(InputDevice* device) {
        devices.insert(std::shared_ptr<InputDevice>(device));
    }
    void clearDevices() {
        devices.clear();
    }

    void assignDevice(InputDevice* device, size_t user_idx) {
        assert(device && user_idx < users.size());
        device->setUser(&users[user_idx]);
    }

    void update() {
        for(auto& d : devices) {
            d->update();
        }
    }
};


inline InputMgr2& getInputMgr() {
    static InputMgr2 mgr;
    return mgr;
}


#endif
