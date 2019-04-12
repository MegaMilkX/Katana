#ifndef ASSET_PARAMS_HPP
#define ASSET_PARAMS_HPP

#include <map>
#include <string>
#include <fstream>
#include "lib/json.hpp"
#include "util/log.hpp"

class AssetParams {
public:
    void load(const std::string& filename) {
        using json = nlohmann::json;

        std::ifstream file(filename);
        if(!file.is_open()) {
            std::ofstream f(filename, std::ios::out);
            if(!f) {
                LOG_WARN("Failed to create asset param file " << filename);
                return;
            }
            f << "{}";
            f.close();

            file = std::ifstream(filename, std::ios::in);
            if(!file.is_open()) {
                LOG_WARN("Failed to open asset param file " << filename);
                return;
            }
        }

        json j;
        try {
            file >> j;
        } catch (std::exception& ex) {
            LOG_WARN("Failed to parse asset params: " << ex.what());
            return;
        }
        if(!j.is_object()) {
            LOG_WARN("Asset param json root must be an object");
            return;
        }

        parse(j);
    }

    void write(const std::string& filename) {
        using json = nlohmann::json;
        std::ofstream file(filename);
        if(!file.is_open()) {
            LOG_WARN("Failed to write " << filename);
            return;
        }
        json j;
        toJson(j);
        file << j.dump(4);
    }

    void toJson(nlohmann::json& j) {
        j = nlohmann::json::object();
        for(auto& kv : objects) {
            kv.second.toJson(j[kv.first]);
        }
        for(auto& kv : string_values) {
            j[kv.first] = kv.second;
        }
        for(auto& kv : numeric_values) {
            j[kv.first] = kv.second;
        }
        for(auto& kv : bool_values) {
            j[kv.first] = kv.second;
        }
    }

    AssetParams& get_object(const std::string& name) {
        return objects[name];
    }

    bool get_bool(const std::string& name, bool def) {
        if(bool_values.count(name) == 0) {
            bool_values[name] = def;
        }
        return bool_values[name];
    }
    std::string get_string(const std::string& name, const std::string& def) {
        if(string_values.count(name) == 0) {
            string_values[name] = def;
        }
        return string_values[name];
    }
    double get_double(const std::string& name, double def) {
        if(numeric_values.count(name) == 0) {
            numeric_values[name] = def;
        }
        return numeric_values[name];
    }
    float get_float(const std::string& name, float def) {
        if(numeric_values.count(name) == 0) {
            numeric_values[name] = (double)def;
        }
        return (float)numeric_values[name];
    }
    int get_int(const std::string& name, int def) {
        if(numeric_values.count(name) == 0) {
            numeric_values[name] = (double)def;
        }
        return (int)numeric_values[name];
    }
private:
    void parse(nlohmann::json& j) {
        for(auto& kv : j.items()) {
            std::string name = kv.key();
            if(kv.value().is_object()) {
                objects[name].parse(kv.value());
            } else if(kv.value().is_boolean()) {
                bool_values[name] = kv.value().get<bool>();
            } else if(kv.value().is_string()) {
                string_values[name] = kv.value().get<std::string>();
            } else if(kv.value().is_number()) {
                numeric_values[name] = kv.value().get<double>();
            }
        }
    }

    std::map<std::string, AssetParams> objects;
    std::map<std::string, bool> bool_values;
    std::map<std::string, std::string> string_values;
    std::map<std::string, double> numeric_values;
};

#endif
