#ifndef INPUT_TABLE_HPP
#define INPUT_TABLE_HPP

#include <map>
#include <vector>
#include <string>
#include "../lib/json.hpp"

class InputTable {
public:
    bool load(const std::string& filename) {
        using json = nlohmann::json;
        std::ifstream file(filename);
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
                    addAxisKey(axis, id, scale);
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
                    addActionKey(action, id);
                }
            }
        }

        return true;
    }

    void clear() {
        key_to_axis_scale.clear();
        key_to_action.clear();
        axes.clear();
        actions.clear();
    }
    void addAxisKey(const std::string& axis, const std::string& key, float scale) {
        key_to_axis_scale[key].emplace_back(std::make_pair(axis, scale));
    }
    void addActionKey(const std::string& action, const std::string& key) {
        key_to_action[key].emplace_back(action);
    }

    std::vector<std::pair<std::string, float>>& getAxes(const std::string& key) {
        return key_to_axis_scale[key];
    }
    std::vector<std::string>& getActions(const std::string& key) {
        return key_to_action[key];
    }
private:
    std::map<std::string, std::vector<std::pair<std::string, float>>> key_to_axis_scale;
    std::map<std::string, std::vector<std::string>> key_to_action;
    std::map<std::string, int> axes;
    std::map<std::string, int> actions;
};

#endif
