#ifndef MAKE_PREVIEW_HPP
#define MAKE_PREVIEW_HPP

#include "../common/gen/no_preview.png.h"

#include "../common/resource/resource_desc_library.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../lib/stb_image_resize.h"

#include "../common/scene/game_scene.hpp"
#include "../common/resource/model_source.hpp"

#include "../common/renderer.hpp"
#include "../common/scene/controllers/render_controller.hpp"

#include "../common/gen/mesh_sphere.h"

#include "../common/ecs/world.hpp"
#include "../common/ecs/attribs/base_attribs.hpp"
#include "../common/ecs/systems/render.hpp"
#include "../common/ecs/systems/scene_graph.hpp"

template<typename T>
std::shared_ptr<Texture2D> makePreview(std::shared_ptr<T> res) {
    return std::shared_ptr<Texture2D>();
}
template<>
std::shared_ptr<Texture2D> makePreview<Texture2D>(std::shared_ptr<Texture2D> res) {
    std::shared_ptr<Texture2D> out_tex(new Texture2D());
    std::vector<unsigned char> buf;
    float largest_size = res->Width() > res->Height() ? res->Width() : res->Height();
    float size_ratio = 128 / largest_size;
    int w = res->Width() * size_ratio;
    int h = res->Height() * size_ratio;
    buf.resize(w * h * res->getBpp());
    stbir_resize_uint8(
        res->getData(), res->Width(), res->Height(), 0,
        buf.data(), w, h, 0, res->getBpp()
    );
    out_tex->Data(buf.data(), w, h, res->getBpp());
    return out_tex;
}
template<>
std::shared_ptr<Texture2D> makePreview<ecsWorld>(std::shared_ptr<ecsWorld> world) {
    gfxm::mat4 _proj;
    gfxm::mat4 _view;
    gfxm::vec2 vp_sz(128, 128);
    gfxm::vec3 cam_pivot(0, 1.1f, 0);
    float cam_angle_y = gfxm::radian(45.0f);
    float cam_angle_x = gfxm::radian(-25.0f);
    float cam_zoom = 1.5f;

    _proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x / (float)vp_sz.y, 0.01f, cam_zoom * 2.0f);
    gfxm::transform tcam;
    tcam.position(cam_pivot);
    tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
    tcam.rotate(cam_angle_x, tcam.right());
    tcam.translate(tcam.back() * cam_zoom);
    _view = gfxm::inverse(tcam.matrix());

    auto light = world->createEntity();
    light.getAttrib<ecsTranslation>()->setPosition(tcam.matrix() * gfxm::vec4(0, 0, 0, 1));
    light.getAttrib<ecsWorldTransform>();
    light.getAttrib<ecsLightOmni>()->intensity  = 6.0f;
    light.getAttrib<ecsLightOmni>()->radius     = 5.0f;

    RenderViewport vp;
    vp.init(128, 128);
    RendererPBR renderer;

    DrawList dl;
    world->getSystem<ecsysSceneGraph>();
    world->update(1.0f/60.0f);
    world->getSystem<ecsRenderSystem>()->fillDrawList(dl);

    curve<gfxm::vec3> gradient;
    gradient[.0f] = gfxm::vec3(0, 0, 0);
    gradient[.2f] = gfxm::vec3(0, 0, 0);
    gradient[1.0f] = gfxm::vec3(1, 1, 1);
    renderer.setSkyGradient(gradient);
    renderer.draw(&vp, _proj, _view, dl);

    std::vector<unsigned char> buf(128 * 128 * 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vp.getFinalImage());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::shared_ptr<Texture2D> out_tex(new Texture2D());
    out_tex->Data(buf.data(), 128, 128, 3);
    return out_tex;
}
template<>
std::shared_ptr<Texture2D> makePreview<GameScene>(std::shared_ptr<GameScene> res) {
    gfxm::mat4 _proj;
    gfxm::mat4 _view;
    gfxm::vec2 vp_sz(128, 128);
    gfxm::vec3 cam_pivot;
    float cam_angle_y = gfxm::radian(45.0f);
    float cam_angle_x = gfxm::radian(-25.0f);
    float cam_zoom = 5.0f;
    
    res->refreshAabb();
    gfxm::aabb box;
    res->makeAabb(box);
    cam_pivot = (box.from + box.to) * 0.5f;
    cam_zoom = gfxm::length(box.to - box.from);

    _proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x/(float)vp_sz.y, 0.01f, cam_zoom * 2.0f);
    gfxm::transform tcam;
    tcam.position(cam_pivot);
    tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
    tcam.rotate(cam_angle_x, tcam.right());
    tcam.translate(tcam.back() * cam_zoom);
    _view = gfxm::inverse(tcam.matrix());


    RenderViewport vp;
    vp.init(128, 128);
    RendererPBR renderer;
    RenderController* rc = res->getController<RenderController>();
    DrawList dl;
    rc->getDrawList(dl);
    DrawList::DirLight light;
    light.dir = tcam.back();
    light.color = gfxm::vec3(1,1,1);
    light.intensity = 1.0f;
    dl.dir_lights.emplace_back(light);
    /*
    light.dir = tcam.left();
    light.color = gfxm::vec3(1,0,0);
    light.intensity = 10.0f;
    dl.dir_lights.emplace_back(light);
    light.dir = tcam.down();
    light.color = gfxm::vec3(0,0,1);
    light.intensity = 10.0f;
    dl.dir_lights.emplace_back(light);
*/
    curve<gfxm::vec3> gradient;
    gradient[.0f] = gfxm::vec3(0,0,0);
    gradient[.2f] = gfxm::vec3(0,0,0);
    gradient[1.0f] = gfxm::vec3(1,1,1);
    renderer.setSkyGradient(gradient);
    renderer.draw(&vp, _proj, _view, dl, false, true);
    std::vector<unsigned char> buf(128 * 128 * 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vp.getFinalImage());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::shared_ptr<Texture2D> out_tex(new Texture2D());
    out_tex->Data(buf.data(), 128, 128, 3);
    return out_tex;
}
template<>
std::shared_ptr<Texture2D> makePreview<ModelSource>(std::shared_ptr<ModelSource> res) {
    return makePreview(res->world);
}
template<>
std::shared_ptr<Texture2D> makePreview<Material>(std::shared_ptr<Material> mat) {
    std::shared_ptr<Mesh> msh(new Mesh());
    dstream strm;
    std::vector<char> buf(mesh_sphere, mesh_sphere + sizeof(mesh_sphere));
    strm.setBuffer(buf);    
    strm.jump(0);
    msh->deserialize(strm, strm.bytes_available());
    std::shared_ptr<GameScene> scn(new GameScene());
    auto mdl = scn->get<Model>();
    mdl->getSegment(0).mesh = msh;
    mdl->getSegment(0).material = mat;

    return makePreview(scn);
}

#endif
