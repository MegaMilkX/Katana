#ifndef GEN_LIGHTMAP_HPP
#define GEN_LIGHTMAP_HPP

#include "../../renderer.hpp"
#include "../../resource/mesh.hpp"
#include "../../resource/texture2d.h"
#include "../../ecs/attribs/base_attribs.hpp"
#include "../../lib/lightmapper.h"


struct LightmapMeshData {
    int                     tex_width;
    int                     tex_height;
    std::vector<float>      tex_data;

    ecsMeshes::Segment*     segment;
    std::vector<float>      position;
    std::vector<float>      normal;
    std::vector<float>      uv_lightmap;
    std::vector<uint32_t>   indices;
    gfxm::mat4              transform;

    // Result
    std::shared_ptr<Texture2D> lightmap;
};


void GenLightmaps(std::vector<LightmapMeshData>& meshes, RendererPBR* renderer, GBuffer* gbuffer, const DrawList& dl);


#endif
