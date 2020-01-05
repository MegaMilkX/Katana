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
#include "input_axis.hpp"
#include "input_listener.hpp"

#include "../../util/log.hpp"

const float INPUT_REPEAT_DELAY = .5f;
const float INPUT_REPEAT_RATE = .1f;
const float INPUT_TAP_TIME = .1f;
const int INPUT_MAX_ACTION_KEY_COMBINATION = 3;

enum INPUT_PLATFORM_HINT {
    INPUT_PLATFORM_PC,
    INPUT_PLATFORM_DURANGO,
    INPUT_PLATFORM_ORBIS
};


class InputMgr2 {
    std::vector<InputUser>                      users;
    std::set<std::shared_ptr<InputDevice>>      devices;

    std::map<std::string, input_action_uid_t>   action_uids;
    std::vector<InputAction>                    actions;

    std::map<std::string, input_axis_uid_t>     axis_uids;
    std::vector<InputAxis>                      axes;
    
    std::vector<InputListenerBase*>             listener_stack;
    std::queue<InputActionEvent>                deferred_events;
    std::queue<InputAxisEvent>                  deferred_axis_events;

    int                 findListener(InputListenerBase* lis);
    void                updateUserActionBuffers();

public:
                        InputMgr2();
    size_t              userCount() const;
    void                setUserCount(size_t count);
    InputUser*          getUser(size_t idx);

    // Ownership is transferred to input manager
    void                addDevice(InputDevice* device);
    void                clearDevices();
    void                assignDevice(InputDevice* device, size_t user_idx);

    void                setAction(const char* name, const InputAction& action);
    input_action_uid_t  getActionUid(const char* name);
    void                setAxis(const char* name, const InputAxis& axis);
    input_axis_uid_t    getAxisUid(const char* name);

    void                addListener(InputListenerBase* lis);
    void                removeListener(InputListenerBase* lis);
    void                moveListenerToTop(InputListenerBase* lis);

    void                postMessage(const InputActionEvent& msg);
    void                postMessage(const InputAxisEvent& msg);

    void                update(float dt);
};


inline InputMgr2& getInputMgr() {
    static InputMgr2 mgr;
    return mgr;
}


class InputListenerHdl : public InputListenerBase {
    struct InputActionDesc {
        InputActionEventType      type;
        input_action_uid_t        action_uid;
        std::function<void(void)> callback;
    };
    struct InputAxisDesc {
        input_axis_uid_t           axis_uid;
        std::function<void(float)> callback;
    };

    std::vector<InputActionDesc> expected_actions;
    std::vector<InputAxisDesc>   expected_axes;

public:
    InputListenerHdl() {
        getInputMgr().addListener(this);
    }
    virtual ~InputListenerHdl() {
        getInputMgr().removeListener(this);
    }

    void moveToTop() {
        getInputMgr().moveListenerToTop(this);
    }

    void bindPress(const char* name, std::function<void(void)> cb) {
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action press '" << name << "', no such action registered");
            return;
        }
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_PRESS, action_uid, cb });
    }
    void bindPressRepeater(const char* name, std::function<void(void)> cb) {
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action press '" << name << "', no such action registered");
            return;
        }
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_PRESS_REPEAT, action_uid, cb });
    }
    void bindRelease(const char* name, std::function<void(void)> cb) {
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action release '" << name << "', no such action registered");
            return;
        }
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_RELEASE, action_uid, cb });
    }
    void bindTap(const char* name, std::function<void(void)> cb) {
        input_action_uid_t action_uid = getInputMgr().getActionUid(name);
        if(!action_uid) {
            LOG_WARN("Failed to bind input action hold '" << name << "', no such action registered");
            return;
        }
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_TAP, action_uid, cb });
    }
    void bindAxis(const char* name, std::function<void(float)> cb) {
        input_axis_uid_t axis_uid = getInputMgr().getAxisUid(name);
        if(!axis_uid) {
            LOG_WARN("Failed to bind axis callback '" << name << "', no such axis registered");
            return;
        }
        expected_axes.push_back(InputAxisDesc{ axis_uid, cb });
    }

    bool onAction(const InputActionEvent& evt) override {
        for(auto& act : expected_actions) {
            if(act.action_uid == evt.action && act.type == evt.type) {
                if(act.callback) 
                    act.callback();
                return true;
            }
        }
        return false;
    }

    bool onAxis(const InputAxisEvent& evt) override {
        for(auto& ax : expected_axes) {
            if(ax.axis_uid == evt.axis) {
                if(ax.callback) {
                    ax.callback(evt.value);
                }
                return true;
            }
        }
        return false;
    }
};


#endif
