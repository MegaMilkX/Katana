#include "katana_impl.hpp"

#include "platform/platform.hpp"

#include "scene/controllers/render_controller.hpp"
#include "scene/controllers/anim_controller.hpp"
#include "scene/controllers/audio_controller.hpp"
#include "scene/controllers/constraint_ctrl.hpp"
#include "scene/controllers/dynamics_ctrl.hpp"

static float sFrameDelta = .0f;
static float sTime = .0f;

KatanaImpl::KatanaImpl() {
    vp.init(640, 480);
}

void KatanaImpl::run(ktGameMode* sess) {
    _session = sess;
    _session->onStart();
}
void KatanaImpl::stop() {
    //platformShutDown();
}

float KatanaImpl::getTime() { return sTime; }
float KatanaImpl::getDt() { return sFrameDelta; }

void KatanaImpl::update() {
    if(!_session) return;

    _session->getScene().getController<DynamicsCtrl>()->update(1/60.0f);
    _session->getScene().getController<AudioController>()->onUpdate();
    _session->getScene().getController<AnimController>()->onUpdate();

    _session->onUpdate();
    
    unsigned w, h;
    platformGetViewportSize(w, h);
    vp.resize(w, h);

    DrawList dl;
    _session->getScene().getController<RenderController>()->getDrawList(dl);
    Camera* cam = _session->getScene().getController<RenderController>()->getDefaultCamera();
    gfxm::mat4 proj = gfxm::mat4(1.0f);
    gfxm::mat4 view = gfxm::mat4(1.0f);
    if(cam) {
        proj = cam->getProjection(w, h);
        view = cam->getView();
    }
    
    renderer.draw(&vp, proj, view, dl, true);
}

void KatanaImpl::update_time(float delta_time) {
    sFrameDelta = delta_time;
    sTime += delta_time;
}