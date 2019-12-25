#ifndef INPUT_MGR2_HPP
#define INPUT_MGR2_HPP


#include <assert.h>
#include <vector>
#include <set>
#include <queue>
#include <memory>
#include "input_user.hpp"
#include "input_device.hpp"
#include "input_action.hpp"
#include "input_listener.hpp"

#include "../../util/log.hpp"


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
    std::vector<InputUser>                      users;
    std::set<std::shared_ptr<InputDevice>>      devices;

    std::map<std::string, input_action_uid_t>   action_uids;
    std::vector<InputAction>                    actions;

    InputListenerStack                          listener_stack;
    std::queue<InputActionDesc>                 deferred_callbacks;

public:
    InputMgr2();
    size_t userCount() const;
    void setUserCount(size_t count);
    InputUser* getUser(size_t idx);

    // Ownership is transferred to input manager
    void addDevice(InputDevice* device);
    void clearDevices();
    void assignDevice(InputDevice* device, size_t user_idx);

    void setAction(const char* name, const InputAction& action);
    input_action_uid_t getActionUid(const char* name);

    InputListenerStack& getListenerStack();

    void update();
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
        auto& stack = getInputMgr().getListenerStack();
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action press '" << name << "', no such action registered");
            return;
        }
        stack.deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_PRESS, action_uid, cb });
    }
    void bindPressRepeater(const char* name, std::function<void(void)> cb) {
        assert(false);
        // TODO:
    }
    void bindRelease(const char* name, std::function<void(void)> cb) {
        auto& stack = getInputMgr().getListenerStack();
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action release '" << name << "', no such action registered");
            return;
        }
        stack.deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_RELEASE, action_uid, cb });
    }
    void bindHold(const char* name, float time, std::function<void(void)> cb) {
        auto& stack = getInputMgr().getListenerStack();
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action hold '" << name << "', no such action registered");
            return;
        }
        stack.deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_HOLD, action_uid, cb });
    }
    void bindTap(const char* name, std::function<void(void)> cb) {
        auto& stack = getInputMgr().getListenerStack();
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action hold '" << name << "', no such action registered");
            return;
        }
        stack.deref(uid)->expected_actions.push_back(InputActionDesc{ INPUT_ACTION_TAP, action_uid, cb });
    }
};


#endif
