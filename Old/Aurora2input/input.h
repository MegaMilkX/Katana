#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <vector>
#include <map>
#include <functional>

#include <windows.h>

//#include "../general/util.h"
#include "util/split.hpp"
#include "util/log.hpp"
#include "util/filesystem.hpp"
#include "lib/json.hpp"

typedef std::function<void(void)> on_press_callback_t;
typedef std::function<void(void)> on_release_callback_t;
typedef std::function<void(float)> on_axis_callback_t;

class InputObject
{
public:
    InputObject()
    : value(0.0f), prev(0.0f)
    {}

    typedef std::function<void(InputObject*)> iobj_on_press_callback_t;
    typedef std::function<void(InputObject*)> iobj_on_release_callback_t;

    void SetOnPress(iobj_on_press_callback_t cb) { on_press = cb; }
    void SetOnRelease(iobj_on_release_callback_t cb) { on_release = cb; }

    void Set(float v)
    {
        const float press_threshold = 0.9f;

        prev = value;
        value = v;

        if(prev == 0.0f && value > press_threshold)
        {
            if(on_press) on_press(this);
        }
        else if(prev > press_threshold && value == 0.0f)
        {
            if(on_release) on_release(this);
        }
    }

    float Get() { return value; }
private:
    float value;
    float prev;

    iobj_on_press_callback_t on_press;
    iobj_on_release_callback_t on_release;
};

class InputDevice
{
public:
    virtual InputObject* GetInputObject(const std::string& name) = 0;
    virtual void Update() = 0;
};

class Input
{
public:
    void AddDevice(InputDevice* device)
    {
        devices.emplace_back(device);
    }
    void Init()
    {
        LoadBindings();
    }
    void SetAction(const std::string& action, const std::string& input);
    void SetAxis(const std::string& axis, const std::string& input);

    void BindAxis(const std::string& name, on_axis_callback_t cb)
    {
        auto& axis = axes[name];
        axis.callbacks.emplace_back(cb);
    }
    void BindActionPress(const std::string& name, on_press_callback_t cb)
    {
        for(auto in : actions[name].inputs)
        {
            press_callbacks[in].emplace_back(cb);
        }
    }
    void BindActionRelease(const std::string& name, on_release_callback_t cb)
    {
        for(auto in : actions[name].inputs)
        {
            release_callbacks[in].emplace_back(cb);
        }
    }

    void Update()
    {
        for(auto d : devices)
        {
            d->Update();
        }

        for(auto a : axes)
        {
            float axis_value = 0.0f;
            for(auto in : a.second.inputs)
            {
                float v = in.input->Get();
                float s = in.scale;
                axis_value += v * s;
            }
            
            for(auto cb : a.second.callbacks)
            {
                cb(axis_value);
            }
        }
    }

    void SetAxisInput(const std::string& axis, const std::string& key, float scale)
    {
        InputObject* iobj = GetInputObject(key);
        if(!iobj) return;
        //LOG(axis << ", " << key << ", scale: " << scale);

        auto& axis_data = axes[axis];
        axis_data.name = axis;
        axis_data.inputs.emplace_back( AxisInput { key, iobj, scale } );
    }

    void SetActionInput(const std::string& action, const std::string& key)
    {
        InputObject* iobj = GetInputObject(key);
        if(!iobj) return;
        //LOG(action << ", " << key);

        iobj->SetOnPress(std::bind(&Input::_onPress, this, std::placeholders::_1));
        iobj->SetOnRelease(std::bind(&Input::_onRelease, this, std::placeholders::_1));

        actions[action].inputs.emplace_back(iobj);
    }
private:
    void _onPress(InputObject* iobj)
    {
        auto it = press_callbacks.find(iobj);
        if(it == press_callbacks.end()) return;
        for(auto cb : it->second)
        {
            cb();
        }
    }
    void _onRelease(InputObject* iobj)
    {
        auto it = release_callbacks.find(iobj);
        if(it == release_callbacks.end()) return;
        for(auto cb : it->second)
        {
            cb();
        }
    }

    std::map<InputObject*, std::vector<on_press_callback_t>> press_callbacks;
    std::map<InputObject*, std::vector<on_release_callback_t>> release_callbacks;

    struct Action
    {
        std::vector<InputObject*> inputs;
    };
    std::map<std::string, Action> actions;

    struct AxisInput
    {
        std::string name;
        InputObject* input;
        float scale;
    };

    struct Axis
    {
        std::string name;
        std::vector<AxisInput> inputs;
        std::vector<on_axis_callback_t> callbacks;
    };

    std::map<std::string, Axis> axes;
    std::vector<InputDevice*> devices;

    InputObject* GetInputObject(const std::string& key)
    {
        InputObject* iobj = 0;
        for(auto d : devices)
        {
            iobj = d->GetInputObject(key);
            if(iobj) return iobj;
        }
        return 0;
    }

    bool LoadBindings()
    {
        using json = nlohmann::json;
        std::ifstream file(get_module_dir() + "\\bindings.json");
        if(!file.is_open())
        {
            return false;
        }

        json j;
        try
        {
            file >> j;
        }
        catch(std::exception& ex)
        {
            LOG_ERR("Failed to parse bindings json: " << ex.what());
            return false;
        }

        if(!j.is_object())
        {
            LOG_ERR("Bindings json is not an object");
            return false;
        }

        json jaxes = j["Axes"];
        json jactions = j["Actions"];

        if(jaxes.is_object())
        {
            for(json::iterator it = jaxes.begin(); it != jaxes.end(); ++it)
            {
                std::string axis = it.key();
                if(!it.value().is_array())
                    continue;

                for(auto jinput : it.value())
                {
                    if(!jinput.is_object())
                        continue;
                    json jid = jinput["id"];
                    if(!jid.is_string())
                        continue;
                    json jscale = jinput["Scale"];
                    float scale = 1.0f;
                    if(jscale.is_number())
                        scale = jscale.get<float>();
                    std::string id = jid.get<std::string>();
                    SetAxisInput(axis, id, scale);
                }
            }
        }

        if(jactions.is_object())
        {
            for(json::iterator it = jactions.begin(); it != jactions.end(); ++it)
            {
                std::string action = it.key();
                if(!it.value().is_array())
                    continue;

                for(auto jinput : it.value())
                {
                    if(!jinput.is_object())
                        continue;
                    json jid = jinput["id"];
                    if(!jid.is_string())
                        continue;

                    std::string id = jid.get<std::string>();
                    SetActionInput(action, id);
                }
            }
        }

        return true;
    }
};

extern Input gInput;

#endif
