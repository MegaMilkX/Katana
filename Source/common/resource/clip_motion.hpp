#ifndef CLIP_MOTION_HPP
#define CLIP_MOTION_HPP

#include "motion.hpp"

#include "animation.hpp"

#include "../util/imgui_helpers.hpp"

class ClipMotion : public Motion {
    std::shared_ptr<Animation> anim;
public:
    void advance(float dt) override {
        if(!anim || !skeleton) return;
        anim->sample_remapped(samples, cursor * anim->length, anim->getMapping(skeleton.get()));
        cursor += dt * (anim->fps / anim->length);
        if(cursor > 1.0f) {
            cursor -= 1.0f;
        }
    }
    MOTION_TYPE getType() const override { return MOTION_CLIP; }
    void onGui() override {
        imguiResourceTreeCombo("anim", anim, "anm", [](){

        });
    }

    void write(out_stream& out) override {
        DataWriter w(&out);
        if(anim) {
            w.write(anim->Name());
        } else {
            w.write(std::string());
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        std::string anim_name = r.readStr();
        anim = retrieve<Animation>(anim_name);
    }
};

#endif
