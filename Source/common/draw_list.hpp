#ifndef DRAW_LIST_HPP
#define DRAW_LIST_HPP

#include "../common/gl/indexed_mesh.hpp"
#include "../common/gfxm.hpp"
#include <vector>
#include <memory>
#include "../common/resource/material.hpp"

#include "render_state.hpp"

#include "resource/model/mesh_deformer.hpp"

enum DRAW_GROUP {
    DRAW_GROUP_SHADOW_CASTER,
    DRAW_GROUP_EDITOR_SELECTED,
    DRAW_GROUP_COUNT
};
static const int DRAW_GROUP_FLAG_SHADOW_CASTER = 0x1;
static const int DRAW_GROUP_FLAG_EDITOR_SELECTED = 0x2;

class DrawCmd {
public:
    virtual ~DrawCmd() {}

    virtual void bind(RenderState& state) const = 0;

    GLuint vao = 0;
    Material* material = 0;
    size_t indexOffset = 0;
    size_t indexCount = 0;
    Texture2D* lightmap = 0;
    void* object_ptr = 0;
};

class DrawCmdSolid : public DrawCmd {
public:
    virtual void bind(RenderState& state) const {
        state.getProgram()->uploadModelTransform(transform);
    }

    gfxm::mat4 transform;
};

class DrawList {
public:
    struct OmniLight {
        gfxm::vec3 translation = gfxm::vec3(0,0,0);
        gfxm::vec3 color = gfxm::vec3(1,1,1);
        float intensity = 1.0f;
        float radius = 5.0f;
    };
    struct DirLight {
        gfxm::vec3 dir;
        gfxm::vec3 color = gfxm::vec3(1,1,1);
        float intensity = 1.0f;
    };

    struct DeformerToCmd {
        MeshDeformerBase* deformer;
        size_t            drawCmd; // draw command to replace with the result of the deformer
    };
    std::vector<DeformerToCmd> deformers[MESH_DEFORMER_COUNT];

    std::vector<DrawCmdSolid> solids;
    std::vector<OmniLight> omnis;
    std::vector<DirLight> dir_lights;

    std::vector<int> draw_groups[DRAW_GROUP_COUNT];

    void clear() {
        for(int i = 0; i < DRAW_GROUP_COUNT; ++i) {
            draw_groups[i].clear();
        }

        for(int i = 0; i < MESH_DEFORMER_COUNT; ++i) {
            deformers[i].clear();
        }

        solids.clear();
        omnis.clear();
        dir_lights.clear();
    }
private:
    
};

#endif
