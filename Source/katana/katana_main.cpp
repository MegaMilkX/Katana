#include "katana.hpp"

#include "timer.hpp"

#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "../common/scene/controllers/render_controller.hpp"

static float sFrameDelta = .0f;
static float sTime = .0f;

class KatanaImpl : public KatanaApi {
    ktSession* _session = 0;
    RenderViewport vp;
    RendererPBR renderer;

    float frameDelta = .0f;
    float time = .0f;
public:
    KatanaImpl() {
        vp.init(640, 480);
    }

    virtual void run(ktSession* sess) {
        _session = sess;
        _session->start();
    }
    virtual void stop() {
        //platformShutDown();
    }

    virtual float getTime() { return sTime; }
    virtual float getDt() { return sFrameDelta; }

    void update() {
        if(!_session) return;
        
        _session->onUpdate();
        
        unsigned w, h;
        platformGetViewportSize(w, h);
        vp.resize(w, h);

        DrawList dl;
        _session->getScene()->getController<RenderController>()->getDrawList(dl);
        Camera* cam = _session->getScene()->getController<RenderController>()->getDefaultCamera();
        gfxm::mat4 proj = gfxm::mat4(1.0f);
        gfxm::mat4 view = gfxm::mat4(1.0f);
        if(cam) {
            proj = cam->getProjection(w, h);
            view = cam->getView();
        }
        
        renderer.draw(&vp, proj, view, dl, true);
    }
};

int main(int argc, char **argv) {
    if(!platformInit()) {
        LOG_ERR("Failed to init platform");
    }

    KatanaImpl kt;
    timer frameTimer;

    ktStartup(&kt);
    while(!platformIsShuttingDown()) {
        frameTimer.start();
        platformUpdate();

        kt.update();

        platformSwapBuffers();
        sFrameDelta = frameTimer.end();
        sTime += sFrameDelta;
    }

    platformCleanup();
}