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

        json j_albedo = j["albedo"];
        if(j_albedo.is_string()) {
            std::string albedo_name = j_albedo.get<std::string>();
            this->albedo = getResource<Texture2D>(albedo_name);
        } else {
            LOG_WARN("albedo value must be a string");
        }

        return true;
    }
    virtual bool Serialize(std::vector<unsigned char>& data) {
        return false;
    }
};

#endif
