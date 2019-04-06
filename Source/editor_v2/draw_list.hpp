#ifndef DRAW_LIST_HPP
#define DRAW_LIST_HPP

#include "../common/gl/indexed_mesh.hpp"
#include "../common/gfxm.hpp"
#include <vector>
#include <memory>
#include "../common/resource/material.hpp"
#include "../editor_v2/components/light_source.hpp"

class DrawList {
public:
    struct Solid {
        GLuint vao;
        Material* material;
        size_t indexCount;
        gfxm::mat4 transform;
    };
    struct Skin {
        GLuint vao;
        Material* material;
        size_t indexCount;
        gfxm::mat4 transform;
        std::vector<gfxm::mat4> bone_transforms;
        std::vector<gfxm::mat4> bind_transforms;
    };

    void add(const Solid& s) {
        solids.emplace_back(s);
    }
    size_t solidCount() const {
        return solids.size();
    }
    const Solid* getSolid(size_t i) const {
        return &solids[i];
    }

    void add(const Skin& s) {
        skins.emplace_back(s);
    }
    size_t skinCount() const {
        return skins.size();
    }
    const Skin* getSkin(size_t i) const {
        return &skins[i];
    }

    size_t omniLightCount() const {
        return omnis.size();
    }
    void add(OmniLight* l) {
        omnis.emplace_back(l);
    }
    const OmniLight* getOmniLight(size_t i) const {
        return omnis[i];
    }
private:
    std::vector<Solid> solids;
    std::vector<Skin> skins;
    std::vector<OmniLight*> omnis;
};

#endif
