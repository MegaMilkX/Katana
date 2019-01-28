#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "resource.h"
#include "../gfxm.hpp"
#include "texture2d.h"
#include "../lib/json.hpp"
#include "../util/log.hpp"
#include "../resource/resource_factory.h"

class Material : public Resource {
public:
    gfxm::vec3 tint;

    std::shared_ptr<Texture2D> albedo;
    std::shared_ptr<Texture2D> normal;
    std::shared_ptr<Texture2D> metallic;
    std::shared_ptr<Texture2D> roughness;

    virtual bool Build(DataSourceRef r) {
        std::vector<char> data;
        if(r->Size() == 0) return false;
        data.resize((size_t)r->Size());
        r->ReadAll((char*)data.data());

        using json = nlohmann::json;
        json j;
        try {
            j = json::parse((char*)data.data(), (char*)data.data() + data.size());
        } catch(std::exception& e) {
            LOG_WARN("Material json parse error: " << e.what());
            return false;
        }

        getMap(j, "albedo", albedo);
        getMap(j, "normal", normal);
        getMap(j, "metallic", metallic);
        getMap(j, "roughness", roughness);

        return true;
    }
    virtual bool Serialize(std::vector<unsigned char>& data) {
        return false;
    }

    virtual void serialize(std::ostream& out) {
        using json = nlohmann::json;
        json j;
        if(albedo) {
            j["albedo"] = albedo->Name();
        }
        if(normal) {
            j["normal"] = normal->Name();
        }
        if(metallic) {
            j["metallic"] = metallic->Name();
        }
        if(roughness) {
            j["roughness"] = roughness->Name();
        }

        out << j;
    }

private:
    void getMap(nlohmann::json& j, const std::string& name, std::shared_ptr<Texture2D>& ptr) {
        nlohmann::json j_ = j[name];
        if(j_.is_string()) {
            std::string name = j_.get<std::string>();
            ptr = getResource<Texture2D>(name);
        } else {
            LOG_WARN(name << " value must be a string");
        }
    }
};

#endif
