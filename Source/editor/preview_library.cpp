#include "preview_library.hpp"

#include "../common/gen/no_preview.png.h"

#include "../common/resource/resource_desc_library.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../lib/stb_image_resize.h"

#include "../common/scene/game_scene.hpp"
#include "../common/resource/model_source.hpp"

#include "../common/renderer.hpp"
#include "../common/scene/controllers/render_controller.hpp"

#include "../common/gen/mesh_sphere.h"

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
    light.intensity = 10.0f;
    dl.dir_lights.emplace_back(light);
    light.dir = tcam.left();
    light.color = gfxm::vec3(1,0,0);
    light.intensity = 10.0f;
    dl.dir_lights.emplace_back(light);
    light.dir = tcam.down();
    light.color = gfxm::vec3(0,0,1);
    light.intensity = 10.0f;
    dl.dir_lights.emplace_back(light);

    curve<gfxm::vec3> gradient;
    gradient[.0f] = gfxm::vec3(0,0,0);
    gradient[.2f] = gfxm::vec3(0,0,0);
    gradient[1.0f] = gfxm::vec3(1,1,1);
    renderer.setSkyGradient(gradient);
    renderer.draw(&vp, _proj, _view, dl, false, true);
    renderer.draw(&vp, _proj, _view, dl, false, true);
    std::vector<unsigned char> buf(128 * 128 * 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vp.getFinalImage());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_BYTE, buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::shared_ptr<Texture2D> out_tex(new Texture2D());
    out_tex->Data(buf.data(), 128, 128, 3);
    return out_tex;
}
template<>
std::shared_ptr<Texture2D> makePreview<ModelSource>(std::shared_ptr<ModelSource> res) {
    return makePreview(res->scene);
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

PreviewLibrary::PreviewLibrary() {
    no_preview_tex.reset(new Texture2D());
    std::vector<char> buf((char*)no_preview_png, (char*)no_preview_png + sizeof(no_preview_png));
    dstream strm;
    strm.setBuffer(buf);
    strm.jump(0);
    no_preview_tex->deserialize(strm, strm.bytes_available());

    int rc = sqlite3_open_v2("meta.db", &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    if(rc) {
        LOG_WARN("Failed to open meta database: " << sqlite3_errmsg(_db));
    }
    sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS thumbnails (resource_id TEXT PRIMARY KEY, png BLOB)", 0, 0, 0);
}
PreviewLibrary::~PreviewLibrary() {
    sqlite3_close_v2(_db);
}

std::shared_ptr<Texture2D> PreviewLibrary::getPreview(const std::string& res_path) {
    std::shared_ptr<Texture2D> preview_tex = no_preview_tex;
    auto it = loaded_thumbs.find(res_path);
    if(it == loaded_thumbs.end()) {
        sqlite3_stmt* stmt = 0;
        int rc = sqlite3_prepare_v2(
            _db, 
            MKSTR("SELECT DISTINCT png FROM thumbnails WHERE resource_id = ?").c_str(),
            -1, &stmt, NULL
        );
        if(rc) {
            LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
        }
        rc = sqlite3_bind_text(stmt, 1, res_path.c_str(), res_path.size(), 0);
        if(rc) {
            LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
        }
        if(sqlite3_step(stmt) == SQLITE_ROW) {
            int blob_size = sqlite3_column_bytes(stmt, 0);
            const void* blob = sqlite3_column_blob(stmt, 0);

            std::shared_ptr<Texture2D> tex(new Texture2D());
            dstream strm;
            std::vector<char> buf((const char*)blob, (const char*)blob + blob_size);
            strm.setBuffer(buf);
            strm.jump(0);
            tex->deserialize(strm, strm.bytes_available());
            loaded_thumbs[res_path] = tex;
            preview_tex = tex;
        } else {
            rttr::type type = ResourceDescLibrary::get()->findType(res_path);
            std::shared_ptr<Texture2D> texture;
            if(type == rttr::type::get<Texture2D>()) {
                texture = makePreview(retrieve<Texture2D>(res_path));
            } else if(type == rttr::type::get<GameScene>()) {
                texture = makePreview(retrieve<GameScene>(res_path));
            } else if(type == rttr::type::get<ModelSource>()) {
                texture = makePreview(retrieve<ModelSource>(res_path));
            } else if(type == rttr::type::get<Material>()) {
                texture = makePreview(retrieve<Material>(res_path));
            }

            if(texture) {
                sqlite3_stmt* stmt = 0;
                int rc = sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO thumbnails(resource_id, png) VALUES(?, ?)", -1, &stmt, NULL);
                if(rc) {
                    LOG_WARN("sqlite3_prepare_v2 failed: " << sqlite3_errmsg(_db));
                }
                rc = sqlite3_bind_text(stmt, 1, res_path.data(), res_path.size(), 0);
                if(rc) {
                    LOG_WARN("sqlite3_bind_text failed: " << sqlite3_errmsg(_db));
                }
                dstream data_strm;
                texture->serialize(data_strm);
                std::vector<char> data_buf = data_strm.getBuffer();
                rc = sqlite3_bind_blob(stmt, 2, data_buf.data(), data_buf.size(), 0);
                if(rc) {
                    LOG_WARN("sqlite3_bind_blob failed: " << sqlite3_errmsg(_db));
                }
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                loaded_thumbs[res_path] = texture;
                preview_tex = texture;
            } else {
                loaded_thumbs[res_path] = no_preview_tex;
                preview_tex = no_preview_tex;
            }
        }
        sqlite3_finalize(stmt);
    } else {
        preview_tex = it->second;
    }

    return preview_tex;
}

std::shared_ptr<Texture2D> PreviewLibrary::getPreviewPlaceholder() {
    return no_preview_tex;
}