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
    gfxm::vec3 tint = gfxm::vec3(1,1,1);

    std::shared_ptr<Texture2D> albedo;
    std::shared_ptr<Texture2D> normal;
    std::shared_ptr<Texture2D> metallic;
    std::shared_ptr<Texture2D> roughness;

    virtual void serialize(out_stream& out) {
        using json = nlohmann::json;
        json j;
        j = json::object();

        j["tint"] = { tint.x, tint.y, tint.z };
        
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

        out.write(j.dump());
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        std::vector<char> buf;
        buf.resize(sz);
        in.read((char*)buf.data(), buf.size());

        using json = nlohmann::json;
        json j;
        try {
            j = json::parse((char*)buf.data(), (char*)buf.data() + sz);
        } catch(std::exception& e) {
            LOG_WARN("Material json parse error: " << e.what());
            return false;
        }

        json jtint = j["tint"];
        if(jtint.is_array()) {
            tint = gfxm::vec3(jtint[0].get<float>(), jtint[1].get<float>(), jtint[2].get<float>());
        }

        getMap(j, "albedo", albedo);
        getMap(j, "normal", normal);
        getMap(j, "metallic", metallic);
        getMap(j, "roughness", roughness);

        return true;
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
