#ifndef INPUT_HPP
#define INPUT_HPP

#include <functional>
#include <stack>
#include "../util/log.hpp"
#include "../util/filesystem.hpp"
#include "../lib/json.hpp"

class InputMgr;
class InputNode {
public:
    typedef std::function<void(InputNode*)> on_press_callback_t;
    typedef std::function<void(InputNode*)> on_release_callback_t;
private:
    int16_t value = 0;
    int16_t prev = 0;
    InputMgr* mgr = 0;
    on_press_callback_t on_press;
    on_release_callback_t on_release;
public:
    void setOnPress(on_press_callback_t cb) { on_press = cb; }
    void setOnRelease(on_release_callback_t cb) { on_release = cb; }
    void set(int16_t v) {
        const int16_t press_threshold = std::numeric_limits<int16_t>().max();
        prev = value;
        value = v;
        if(prev == 0 && abs(value) >= press_threshold) {
            if(on_press) on_press(this);
        } else if(abs(prev) >= press_threshold && value == 0) {
            if(on_release) on_release(this);
        }
        if(prev != value) {
            // TODO: Trigger on_axis
        }
    }
};

class InputDevice {
public:
    virtual void init(InputMgr* mgr) = 0;
    virtual InputNode* getInputNode(const std::string& name) = 0;
    virtual void update() = 0;
};

class InputMgr {
public:
    typedef int scope_t;
    typedef std::function<void(void)> on_press_callback_t;
    typedef std::function<void(void)> on_release_callback_t;
    typedef std::function<void(float)> on_axis_callback_t;
    typedef int key_t;

    struct Action {
        std::vector<InputNode*> inputs;
    };
    struct AxisInput {
        std::string name;
        InputNode* input;
        float scale;
    };
    struct Axis {
        std::string name;
        std::vector<AxisInput> inputs;
        std::vector<on_axis_callback_t> callbacks;
    };

    void addDevice(InputDevice* device);

    bool loadBindings();

    void bindAxis(const std::string& name, on_axis_callback_t cb, scope_t scope = 0) {
        auto& axis = axes[name];
        axis.callbacks.emplace_back(cb);
    }
    void bindActionPress(const std::string& name, on_press_callback_t cb, scope_t scope = 0) {
        for(auto in : actions[name].inputs) {
            press_callbacks[in].emplace_back(cb);
        }
    }
    void bindActionRelease(const std::string& name, on_release_callback_t cb, scope_t scope = 0) {
        for(auto in : actions[name].inputs) {
            release_callbacks[in].emplace_back(cb);
        }
    }

    void pushScope(scope_t s);
    void popScope();

    void update();

    InputNode* getInputNode(const std::string& name) {
        InputNode* node = 0;
        for(auto d : devices) {
            node = d->getInputNode(name);
            if(node) return node;
        }
        return 0;
    }

    void setAxis(const std::string& axis_name, const std::string& key, float scale) {
        InputNode* n = getInputNode(key);
        if(!n) {
            LOG_WARN("Axis '" << key << "' not found");
            return;
        }
        auto& axis = axes[axis_name];
        axis.name = axis_name;
        axis.inputs.emplace_back(AxisInput{key, n, scale});
    }
    void setAction(const std::string& action, const std::string& key) {
        InputNode* n = getInputNode(key);
        if(!n) {
            LOG_WARN("Key '" << key << "' not found");
            return;
        }
        n->setOnPress(std::bind(&InputMgr::_onPress, this, std::placeholders::_1));
        n->setOnRelease(std::bind(&InputMgr::_onRelease, this, std::placeholders::_1));
        actions[action].inputs.emplace_back(n);
    }

private:
    std::stack<scope_t> scope_stack;
    std::vector<std::shared_ptr<InputDevice>> devices;

    std::map<std::string, Action> actions;
    std::map<std::string, Axis> axes;

    std::map<InputNode*, std::vector<on_press_callback_t>> press_callbacks;
    std::map<InputNode*, std::vector<on_release_callback_t>> release_callbacks;

    void _onPress(InputNode* iobj)
    {
        auto it = press_callbacks.find(iobj);
        if(it == press_callbacks.end()) return;
        for(auto cb : it->second) {
            cb();
        }
    }
    void _onRelease(InputNode* iobj)
    {
        auto it = release_callbacks.find(iobj);
        if(it == release_callbacks.end()) return;
        for(auto cb : it->second) {
            cb();
        }
    }
};

inline InputMgr& input() {
    static InputMgr ipt;
    return ipt;
}

#endif
