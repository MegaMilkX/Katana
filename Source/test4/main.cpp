
void getDrawList();


void startSession();
void endSession();


void updateBehaviors();
void updateDynamics();
void updateCollision();
void updateAudio();
void updateConstraints();
void renderToScreen();

#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include <iostream>
#include "core/session.hpp"
#include "sess_gameplay.hpp"

#include "../common/renderer.hpp"
#include "../common/render_viewport.hpp"

#include "../common/scene/game_scene.hpp"
#include "../editor/scene_object_from_fbx.hpp"
#include "../common/scene/controllers/render_controller.hpp"

#include "../common/gen/psx_f.glsl.h"
#include "../common/gen/psx_skin_v.glsl.h"
#include "../common/gen/psx_solid_v.glsl.h"

class RendererPSX : public Renderer {
    gl::ShaderProgram* prog_solid;
    gl::ShaderProgram* prog_skin;
public:
    RendererPSX() {
        prog_solid = ShaderFactory::getOrCreate(
            "psx_solid", (char*)psx_solid_v_glsl, (char*)psx_f_glsl
        );
        prog_skin = ShaderFactory::getOrCreate(
            "psx_skin", (char*)psx_skin_v_glsl, (char*)psx_f_glsl
        );
    }
    virtual void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list, bool draw_final_on_screen = false) {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());
        getState().ubufCommon3d.upload(uCommon3d{view, proj});
        getState().bindUniformBuffers();
        glClear(GL_DEPTH_BUFFER_BIT);

        drawMultiple(
            prog_solid,
            draw_list.solids.data(),
            draw_list.solids.size()
        );
        drawMultiple(
            prog_skin,
            draw_list.skins.data(),
            draw_list.skins.size()
        );

        endFrame();
    }
};


void main() {
    if(!platformInit()) {
        LOG_ERR("Failed to init platform");
    }

    input().getTable().addAxisKey("MoveHori", "KB_D", 1.0f);
    input().getTable().addAxisKey("MoveHori", "KB_A", -1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_W", 1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_S", -1.0f);
    input().getTable().addAxisKey("MoveCamX", "MOUSE_X", 1.0);
    input().getTable().addAxisKey("MoveCamY", "MOUSE_Y", 1.0);
    input().getTable().addAxisKey("CameraZoom", "MOUSE_SCROLL", 1.0);

    RenderViewport vp;
    vp.init(1280, 720);
    RendererPSX renderer;
    
    GameScene gscn;
    sessGameplay sess;
    sess.setScene(&gscn);

    std::shared_ptr<Mesh> mesh = retrieve<Mesh>("test.msh");

    sess.start();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        
        sess.update();

        DrawList dl;
        gscn.getController<RenderController>()->getDrawList(dl);

        unsigned w, h;
        platformGetViewportSize(w, h);
        //w = 640;
        //h = 480;
        vp.resize(w, h);
        gfxm::mat4 proj, view;
        proj = gscn.getController<RenderController>()->getDefaultCamera()->getProjection(w, h);
        view = gscn.getController<RenderController>()->getDefaultCamera()->getView();
        renderer.draw(
            &vp, 
            proj, 
            view, 
            dl, 
            true
        );

        platformSwapBuffers();
    }
    sess.stop();

    platformCleanup();
}
