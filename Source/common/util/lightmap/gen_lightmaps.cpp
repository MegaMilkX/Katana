#include "gen_lightmaps.hpp"

void GenLightmaps(std::vector<LightmapMeshData>& meshes, RendererPBR* renderer, GBuffer* gbuffer, const DrawList& dl) {
    lm_context* ctx = lmCreate(
        64,
        0.001f, 100.0f, 
        1.0f, 1.0f, 1.0f, 
        2, 0.01f, 
        .0f
    );
    assert(ctx);

    const int bounce_count = 4;
    const int image_cpp = 3;
    std::vector<float> image_tmp;
    for(int i = 0; i < meshes.size(); ++i) {
        auto& m = meshes[i];
        m.tex_data.resize(m.tex_width * m.tex_height * image_cpp);
    }
    for(int i = 0; i < bounce_count; ++i) {
        for(int j = 0; j < meshes.size(); ++j) {
            auto& m = meshes[j];
            memset(m.tex_data.data(), 0, m.tex_data.size() * sizeof(m.tex_data[0]));

            lmSetTargetLightmap(ctx, m.tex_data.data(), m.tex_width, m.tex_height, image_cpp);
            
            lmSetGeometry(
                ctx, (float*)&m.transform, 
                LM_FLOAT, m.position.data(), sizeof(float) * 3, // Position
                LM_FLOAT, m.normal.data(), sizeof(float) * 3, // Normal
                LM_FLOAT, m.uv_lightmap.data(), sizeof(float) * 2, // Lightmap UV
                m.indices.size(), LM_UNSIGNED_INT, m.indices.data()
            ); 

            int vp[4];
            gfxm::mat4 view, proj;
            while (lmBegin(ctx, vp, (float*)&view, (float*)&proj)) {
                renderer->sampleLightmap(gfxm::ivec4(vp[0], vp[1], vp[2], vp[3]), proj, view, dl);
                //renderer->draw(gbuffer, gfxm::ivec4(vp[0], vp[1], vp[2], vp[3]), proj, view, dl, lm_framebuffer_id, true);
                lmEnd(ctx);
                //lmImagePower(m.tex_data.data(), m.tex_width, m.tex_height, image_cpp, 1.0f / 2.2f);
                //lmImageSaveTGAf(MKSTR("lm" << j << "_" << k << ".tga").c_str(), m.tex_data.data(), m.tex_width, m.tex_height, image_cpp);
            }
        }
        for(int j = 0; j < meshes.size(); ++j) {
            auto& m = meshes[j];
            if(image_tmp.size() < m.tex_width * m.tex_height * image_cpp) {
                image_tmp.resize(m.tex_width * m.tex_height * image_cpp);
            }
            lmImageDilate(m.tex_data.data(), image_tmp.data(), m.tex_width, m.tex_height, image_cpp);
            lmImageDilate(image_tmp.data(), m.tex_data.data(), m.tex_width, m.tex_height, image_cpp);

            std::shared_ptr<Texture2D> lmtex(new Texture2D());
            lmtex->Data(m.tex_data.data(), m.tex_width, m.tex_height, image_cpp);
            m.lightmap = lmtex;
            m.segment->lightmap = lmtex;
        }
    }

    lmDestroy(ctx);

    for(int j = 0; j < meshes.size(); ++j) {
        auto& m = meshes[j];
        lmImagePower(m.tex_data.data(), m.tex_width, m.tex_height, image_cpp, 1.0f / 2.2f);
        lmImageSaveTGAf(MKSTR("lm" << j << ".tga").c_str(), m.tex_data.data(), m.tex_width, m.tex_height, image_cpp);
    }
}