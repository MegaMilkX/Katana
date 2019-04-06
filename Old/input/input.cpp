#include "input.hpp"

void InputMgr::addDevice(InputDevice* device) {
    devices.emplace_back(std::shared_ptr<InputDevice>(device));
}

bool InputMgr::loadBindings() {
    using json = nlohmann::json;
    std::ifstream file(get_module_dir() + "\\bindings.json");
    if(!file.is_open()) {
        return false;
    }
    json j;
    try {
        file >> j;
    } catch(std::exception& ex) {
        LOG_ERR("Failed to parse input bindings json: " << ex.what());
        return false;
    }
    if(!j.is_object()) {
        LOG_ERR("Input bindings json is not an object");
        return false;
    }
    json jaxes = j["Axes"];
    json jactions = j["Actions"];
    if(jaxes.is_object()) {
        for(auto it = jaxes.begin(); it != jaxes.end(); ++it) {
            std::string axis = it.key();
            if(!it.value().is_array())
                continue;
            for(auto jinput : it.value()) {
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

                setAxis(axis, id, scale);
            }
        }
    }
    if(jactions.is_object()) {
        for(auto it = jactions.begin(); it != jactions.end(); ++it) {
            std::string action = it.key();
            if(!it.value().is_array())
                continue;
            for(auto jinput : it.value()) {
                if(!jinput.is_object())
                    continue;
                json jid = jinput["id"];
                if(!jid.is_string())
                    continue;
                std::string id = jid.get<std::string>();

                setAction(action, id);
            }
        }
    }
}