#ifndef KT_WORLD_HPP
#define KT_WORLD_HPP

class ktRenderController;
class ktDynamics;
class ktConstraintController;
class ktAudioController;

class ktWorld {
    ktRenderController* render_controller = 0;
    ktDynamicsController* dynamics_controller = 0;
    ktAudioController* audio_controller = 0;
    ktConstraintController* constraint_controller = 0;
    
public:
    ktWorld(
        ktRenderController* rCtrl,
        ktDynamics*         dCtrl,
        ktAudioController*  aCtrl,
        ktConstraintController* cCtrl
    ) {

    }

    void update(float dt) {

    }
};

#endif
