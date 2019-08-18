#ifndef DRAW_LIST_HPP
#define DRAW_LIST_HPP

#include "../common/gl/indexed_mesh.hpp"
#include "../common/gfxm.hpp"
#include <vector>
#include <memory>
#include "../common/resource/material.hpp"

#include "render_state.hpp"

class DrawCmd {
public:
    virtual ~DrawCmd() {}

    virtual void bind(RenderState& state) const = 0;

    GLuint vao = 0;
    Material* material = 0;
    size_t indexOffset = 0;
    size_t indexCount = 0;
};

class DrawCmdSolid : public DrawCmd {
public:
    virtual void bind(RenderState& state) const {
        state.getProgram()->uploadModelTransform(transform);
    }

    gfxm::mat4 transform;
};

class DrawCmdSkin : public DrawCmd {
public:
    virtual void bind(RenderState& state) const {
        state.getProgram()->uploadModelTransform(transform);

        gfxm::mat4 bone_m[SKIN_BONE_LIMIT];
        unsigned bone_count = (std::min)((unsigned)SKIN_BONE_LIMIT, (unsigned)bind_transforms.size());
        for(unsigned i = 0; i < bone_count; ++i) {
            bone_m[i] = bone_transforms[i] * bind_transforms[i];
        }
        uBones bones;
        memcpy(bones.pose, bone_m, sizeof(gfxm::mat4) * bone_count);
        state.ubufBones.upload(bones);
    }

    gfxm::mat4 transform = gfxm::mat4(1.0f);
    std::vector<gfxm::mat4> bone_transforms;
    std::vector<gfxm::mat4> bind_transforms;
};

class DrawList {
public:
    struct OmniLight {
        gfxm::vec3 translation = gfxm::vec3(0,0,0);
        gfxm::vec3 color = gfxm::vec3(1,1,1);
        float intensity = 1.0f;
        float radius = 5.0f;
    };

    std::vector<DrawCmdSolid> solids;
    std::vector<DrawCmdSkin> skins;
    std::vector<OmniLight> omnis;
private:
    
};

#endif
