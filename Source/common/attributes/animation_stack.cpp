#include "animation_stack.hpp"

#include "../scene/node.hpp"
#include "../scene/game_scene.hpp"

AnimationStack::~AnimationStack() {}

void AnimationStack::onCreate() {}

void AnimationStack::play(int layer, const std::string& anim_alias) {
    size_t anim_i = 0;
    for(size_t i = 0; i < anims.size(); ++i) {
        if(anims[i].alias == anim_alias) {
            anim_i = i;
        }
    }

    getLayer(layer).anim_index = anim_i;
    getLayer(layer).cursor = 0.0f;
    getLayer(layer).stopped = false;
    getLayer(layer).blend_over_speed = 0.0f;
}
void AnimationStack::blendOver(int layer, const std::string& anim_alias, float speed) {
    size_t anim_i = 0;
    for(size_t i = 0; i < anims.size(); ++i) {
        if(anims[i].alias == anim_alias) {
            anim_i = i;
        }
    }

    getLayer(layer).blend_target_index = anim_i;
    getLayer(layer).blend_target_cursor = 0.0f;
    getLayer(layer).blend_target_prev_cursor = 0.0f;
    getLayer(layer).blend_over_weight = 0.0f;
    getLayer(layer).blend_over_speed = 1.0f / speed;
}
void AnimationStack::blendOverFromCurrentPose(float blend_time) {
    blend_over_origin_samples = samples;
    blend_over_weight = .0f;
    blend_over_speed = 1.0f / blend_time;
}
bool AnimationStack::layerStopped(int layer, float offset) {
    if(!anims[layers[layer].anim_index].looping) {
        return 
            layers[layer].cursor >= anims[layers[layer].anim_index].anim->length + offset * anims[layers[layer].anim_index].anim->fps;
    } else {
        return false;
    }
}

float AnimationStack::getLengthProportion(int layer_a, int layer_b) {
    float a_len = anims[getLayer(layer_a).anim_index].anim->length;
    float b_len = anims[getLayer(layer_b).anim_index].anim->length;
    return a_len / b_len;
}

void AnimationStack::copy(Attribute* other) {
    if(other->get_type() != get_type()) {
        LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
            get_type().get_name().to_string());
        return;
    }
    AnimationStack* o = (AnimationStack*)other;
    skeleton = o->skeleton;
    anims = o->anims;
    layers = o->layers;
    samples = o->samples;
    root_motion_enabled = o->root_motion_enabled;
}

void AnimationStack::setSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;
    resetSampleBuffer();
    resetSkeletonMapping();
}
void AnimationStack::addAnim(std::shared_ptr<Animation> anim) {
    if(!anim) return;
    size_t slash_pos = anim->Name().find_last_of("/");
    std::string alias = slash_pos == anim->Name().npos ? anim->Name() : anim->Name().substr(slash_pos);
    anims.emplace_back(
        AnimInfo {
            alias,
            anim
        }
    );
    resetSkeletonMapping(anims.back());
}
void AnimationStack::reserveLayers(unsigned count) {
    layers.resize(count);
}
AnimLayer& AnimationStack::addLayer() {
    layers.emplace_back(AnimLayer());
    return layers.back();
}
AnimLayer& AnimationStack::getLayer(unsigned i) {
    if(i >= layers.size()) {
        layers.resize(i+1);
    }
    return layers[i];
}

void AnimationStack::update(float dt) {
    if(!skeleton) return;
    for(auto& c : curve_values) {
        c.second = .0f;
    }

    gfxm::vec3 root_motion_trans = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat root_motion_rot = gfxm::quat(.0f, .0f, .0f, 1.0f);

    for(auto& l : layers) {
        updateLayer(
            l, dt,
            root_motion_trans,
            root_motion_rot
        );
    }

    if(blend_over_speed > .0f) {
        blend_over_weight += dt * blend_over_speed;
        if(blend_over_weight > 1.0f) blend_over_weight = 1.0f;

        for(size_t i = 0; i < samples.size() && i < blend_over_origin_samples.size(); ++i) {
            samples[i].t = gfxm::lerp(blend_over_origin_samples[i].t, samples[i].t, blend_over_weight);
            samples[i].r = gfxm::slerp(blend_over_origin_samples[i].r, samples[i].r, blend_over_weight);
            samples[i].s = gfxm::lerp(blend_over_origin_samples[i].s, samples[i].s, blend_over_weight);
        }

        if(blend_over_weight >= 1.0f) {
            blend_over_speed = .0f;
        }
    }

    // Update transforms
    // TODO: Optimize
    for(size_t i = 0; i < skeleton->boneCount(); ++i) {
        Skeleton::Bone& b = skeleton->getBone(i);
        auto so = getOwner()->findObject(b.name);
        if(!so) continue;
        auto trans = so->getTransform();
        trans->setPosition(samples[i].t);
        trans->setRotation(samples[i].r);
        trans->setScale(samples[i].s);
    }

    // Apply root motion
    if(root_motion_enabled) {
        auto t = getOwner()->getTransform();

        t->rotate(root_motion_rot);

        gfxm::vec4 t4 = gfxm::vec4(
            root_motion_trans.x,
            root_motion_trans.y,
            root_motion_trans.z,
            0.0f
        );
        gfxm::mat4 root_m4 = t->getWorldTransform();
        t4 = (root_m4) * t4;

        t->translate(t4);
    }
}

void AnimationStack::setEventCallback(const std::string& name, std::function<void(void)> cb) {
    callbacks[name] = cb;
}
float AnimationStack::getCurveValue(const std::string& name) {
    return curve_values[name];
}

void AnimationStack::write(SceneWriteCtx& o) {
    o.write<uint8_t>(root_motion_enabled);
    
    o.write<uint32_t>(layers.size());
    for(size_t i = 0; i < layers.size(); ++i) {
        auto& l = layers[i];
        o.write<uint32_t>(l.anim_index);
        o.write<uint32_t>(l.mode);
        o.write(l.cursor);
        o.write(l.speed);
        o.write(l.weight);
    }

    o.write<uint32_t>(anims.size());
    for(size_t i = 0; i < anims.size(); ++i) {
        AnimInfo& a = anims[i];
        o.write(a.alias);
        if(a.anim) {
            o.write(a.anim->Name());
        } else {
            o.write(std::string());
        }
        o.write<uint8_t>(a.looping);
        o.write<uint32_t>(a.bone_remap.size());
        for(auto& v : a.bone_remap) {
            o.write<uint32_t>(v);
        }
    }

    if(skeleton) {
        o.write(skeleton->Name());
    } else {
        o.write(std::string());
    }
}

void AnimationStack::read(SceneReadCtx& r) {
    root_motion_enabled = r.read<uint8_t>();
    
    layers.resize(r.read<uint32_t>());
    for(size_t i = 0; i < layers.size(); ++i) {
        auto& l = layers[i];
        l.anim_index = r.read<uint32_t>();
        l.mode = (ANIM_BLEND_MODE)r.read<uint32_t>();
        l.cursor = r.read<float>();
        l.speed = r.read<float>();
        l.weight = r.read<float>();
    }

    uint32_t anim_count = r.read<uint32_t>();
    for(size_t i = 0; i < anim_count; ++i) {
        std::string alias = r.readStr();
        auto anim = retrieve<Animation>(r.readStr());
        bool looping = (bool)r.read<uint8_t>();
        std::vector<size_t> bone_remap;

        uint32_t bone_remap_sz = r.read<uint32_t>();
        for(uint32_t j = 0; j < bone_remap_sz; ++j) {
            auto v = r.read<uint32_t>();
            bone_remap.emplace_back(v);
        }

        if(anim) {
            anims.emplace_back(AnimInfo());
            anims.back().alias = alias;
            anims.back().anim = anim;
            anims.back().looping = looping;
            anims.back().bone_remap = bone_remap;
        }
    }

    setSkeleton(retrieve<Skeleton>(r.readStr()));
}

// ==== Private ===================

void AnimationStack::updateLayer(
    AnimLayer& l, 
    float dt,
    gfxm::vec3& rm_pos_final,
    gfxm::quat& rm_rot_final
) {
    if(l.anim_index >= anims.size()) return;

    auto& anim_info = anims[l.anim_index];
    auto& anim = anim_info.anim;
    float duration = anim_info.anim->length;
    float fps = anim_info.anim->fps;

    float cursor_prev = l.cursor;
    if(!l.stopped) {
        l.cursor += dt * fps * l.speed;
        if(l.cursor >= duration) {
            if(anim_info.looping) {
                l.cursor -= duration;
            } else {
                l.cursor = duration;
                l.stopped = true;
            }
        }
    }
    if(l.weight == 0.0f) return;

    blendAnim(
        anim_info.anim.get(),
        anim_info.bone_remap,
        l.cursor, cursor_prev,
        l.mode, l.weight,
        samples,
        root_motion_enabled,
        rm_pos_final,
        rm_rot_final,
        [this](const std::string& evt){
            auto it = callbacks.find(evt);
            if(it != callbacks.end()) {
                it->second();
            }
        }, 0.0f
    );

    if(l.blend_over_speed > 0.0f) {
        float fps = anims[l.blend_target_index].anim->fps;
        l.blend_over_weight += dt * l.blend_over_speed;
        if(l.blend_over_weight > 1.0f) l.blend_over_weight = 1.0f;

        blendAnim(
            anims[l.blend_target_index].anim.get(),
            anims[l.blend_target_index].bone_remap,
            l.blend_target_cursor, l.blend_target_prev_cursor,
            ANIM_MODE_BLEND, l.blend_over_weight,
            samples,
            root_motion_enabled,
            rm_pos_final,
            rm_rot_final
        );

        l.blend_target_prev_cursor = l.blend_target_cursor;
        l.blend_target_cursor += dt * fps * l.speed;
        if(l.blend_over_weight >= 1.0f) {
            l.anim_index = l.blend_target_index;
            l.cursor = l.blend_target_cursor;
            l.blend_over_speed = 0.0f;
            l.stopped = false;
        }
    }
}

void AnimationStack::blendAnim(
    Animation* anim,
    std::vector<size_t>& bone_remap,
    float cursor, 
    float cursor_prev,
    ANIM_BLEND_MODE mode,
    float weight,
    std::vector<AnimSample>& samples,
    bool enable_root_motion,
    gfxm::vec3& rm_pos_final,
    gfxm::quat& rm_rot_final,
    std::function<void(const std::string&)> evt_callback,
    float event_threshold
) {
    gfxm::vec3 root_motion_pos_delta;
    gfxm::quat root_motion_rot_delta;
    bool do_root_motion = anim->root_motion_enabled;
    if(do_root_motion) {
        gfxm::vec3 delta_pos =  anim->getRootMotionNode().t.delta(cursor_prev, cursor);
        gfxm::vec3 delta_pos4 = gfxm::vec4(delta_pos.x, delta_pos.y, delta_pos.z, 0.0f);
        gfxm::quat delta_q = anim->getRootMotionNode().r.delta(cursor_prev, cursor);

        root_motion_pos_delta = delta_pos4;
        root_motion_rot_delta = delta_q;
    }

    switch(mode) {
    case ANIM_MODE_NONE:
        anim->sample_remapped(samples, cursor, bone_remap);
        if(enable_root_motion) {
            rm_pos_final = root_motion_pos_delta;
            rm_rot_final = root_motion_rot_delta;
        }
        for(size_t i = 0; i < anim->curveCount(); ++i) {
            curve_values[anim->getCurveName(i)] = anim->getCurve(i).at(cursor);
        }
        break;
    case ANIM_MODE_BLEND:
        anim->blend_remapped(samples, cursor, weight, bone_remap);
        if(enable_root_motion) {
            rm_pos_final = gfxm::lerp(rm_pos_final, root_motion_pos_delta, weight);
            rm_rot_final = gfxm::slerp(rm_rot_final, root_motion_rot_delta, weight);
        }
        for(size_t i = 0; i < anim->curveCount(); ++i) {
            float b = anim->getCurve(i).at(cursor);
            float a = curve_values[anim->getCurveName(i)];
            curve_values[anim->getCurveName(i)] = gfxm::lerp(a, b, weight);
        }
        break;
    case ANIM_MODE_ADD:
        anim->additive_blend_remapped(samples, cursor, weight, bone_remap);
        // NOTE: Additive root motion is untested
        if(enable_root_motion) {
            rm_pos_final = gfxm::lerp(rm_pos_final, rm_pos_final + root_motion_pos_delta, weight);
            rm_rot_final = gfxm::slerp(rm_rot_final, root_motion_rot_delta * rm_rot_final, weight);
        }
        for(size_t i = 0; i < anim->curveCount(); ++i) {
            float b = anim->getCurve(i).at(cursor);
            float a = curve_values[anim->getCurveName(i)];
            curve_values[anim->getCurveName(i)] = a + b * weight;
        }
        break;
    }
    if(weight >= event_threshold) {
        anim->fireEvents(cursor_prev, cursor, evt_callback, weight);
    }
}

void AnimationStack::resetSkeletonMapping() {
    if(!skeleton) return;
    for(auto& a : anims) {
        resetSkeletonMapping(a);
    }
}
void AnimationStack::resetSkeletonMapping(AnimInfo& anim_info) {
    if(!skeleton) return;

    anim_info.bone_remap.resize(anim_info.anim->nodeCount());
    for(size_t i = 0; i < skeleton->boneCount(); ++i) {
        Skeleton::Bone& b = skeleton->getBone(i);
        int32_t bone_index = (int32_t)i;
        int32_t node_index = anim_info.anim->getNodeIndex(b.name);
        if(node_index >= 0) {
            anim_info.bone_remap[node_index] = bone_index;
        }
    }
}
void AnimationStack::resetSampleBuffer() {
    if(!skeleton) return;
    samples.resize(skeleton->boneCount());
    for(size_t i = 0; i < skeleton->boneCount(); ++i) {
        Skeleton::Bone& b = skeleton->getBone(i);

        gfxm::vec3 _position = gfxm::vec3(b.bind_pose[3].x, b.bind_pose[3].y, b.bind_pose[3].z);
        gfxm::mat3 rotMat = gfxm::to_orient_mat3(b.bind_pose);
        gfxm::quat _rotation = gfxm::to_quat(rotMat);
        gfxm::vec3 right = b.bind_pose[0];
        gfxm::vec3 up = b.bind_pose[1];
        gfxm::vec3 back = b.bind_pose[2];
        gfxm::vec3 _scale = gfxm::vec3(right.length(), up.length(), back.length());

        samples[i].t = _position;
        samples[i].r = _rotation;
        samples[i].s = _scale;
    }
}