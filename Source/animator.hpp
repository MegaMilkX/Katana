#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "component.hpp"
#include "resource/animation.hpp"
#include "resource/skeleton.hpp"
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/maths/soa_transform.h>

#include "skeleton_anim_layer.hpp"

class Animator : public Component {
    CLONEABLE
    friend SkeletonAnimLayer;
public:
    Animator() {
        auto& layer0 = addLayer();
        layer0.anim_index = 0;
    }

    ~Animator() {
    }

    struct Layer {
        enum BlendMode {
            BASE,
            BLEND,
            ADD,
            BLEND_MODE_LAST
        };
        static std::string blendModeString(BlendMode m) {
            std::string r = "UNKNOWN";
            switch(m) {
            case BASE:
                r = "BASE";
                break;
            case BLEND:
                r = "BLEND";
                break;
            case ADD:
                r = "ADD";
                break;
            }
            return r;
        }

        int anim_index;
        BlendMode mode;
        float cursor;
        float speed;
        float weight;
    };

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
    }

    void addAnim(std::shared_ptr<Animation> anim) {
        anims.emplace_back(anim);
    }

    Layer& addLayer() {
        Layer l = { 0 };
        l.weight = 1.0f;
        l.speed = 1.0f;
        layers.emplace_back(l);
        return layers.back();
    }

    void Update(float dt) {
        if(!this->skeleton) {
            return;
        }
        ozz::animation::Skeleton* skeleton = this->skeleton->getOzzSkeleton();

        ozz::animation::SamplingCache* cache = 
            ozz::memory::default_allocator()->New<ozz::animation::SamplingCache>(skeleton->num_joints());
        ozz::Range<ozz::math::SoaTransform> locals_fin = 
            ozz::memory::default_allocator()->AllocateRange<ozz::math::SoaTransform>(skeleton->num_soa_joints());
        ozz::Range<ozz::math::Float4x4> models =
            ozz::memory::default_allocator()->AllocateRange<ozz::math::Float4x4>(skeleton->num_joints());
        
        // Final root motion translation
        gfxm::vec3 rm_pos_final = gfxm::vec3(0.0f, 0.0f, 0.0f);
        // Final root motion rotation
        gfxm::quat rm_rot_final = gfxm::quat(0.0f, 0.0f, 0.0f, 1.0f);

        for(auto& l : layers) {
            if(l.anim_index >= anims.size()) {
                continue;
            }
            if(anims[l.anim_index]->anim->duration() == 0.0f) {
                continue;
            }
            ozz::Range<ozz::math::SoaTransform> locals = 
                ozz::memory::default_allocator()->AllocateRange<ozz::math::SoaTransform>(skeleton->num_soa_joints());
            float layer_cursor_prev = l.cursor;
            l.cursor += dt * 60.0f;
            if(l.cursor > anims[l.anim_index]->anim->duration()) {
                l.cursor -= anims[l.anim_index]->anim->duration();
            }

            Transform* root_motion_transform = 0;
            if(!anims[l.anim_index]->root_motion_node.empty()) {
                SceneObject* root_so = getObject()->findObject(anims[l.anim_index]->root_motion_node);
                if(root_so) {
                    root_motion_transform = 
                        root_so->get<Transform>();
                }
            }

            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = anims[l.anim_index]->anim;
            sampling_job.cache = cache;
            sampling_job.output = locals;
            sampling_job.ratio = l.cursor / anims[l.anim_index]->anim->duration();

            if(!sampling_job.Run()) {
                LOG_WARN("Sample job failed");
                return;
            }

            gfxm::vec3 root_motion_pos_delta;
            gfxm::quat root_motion_rot_delta;
            bool do_root_motion = anims[l.anim_index]->root_motion_enabled && root_motion_transform;
            if(do_root_motion) {
                gfxm::vec3 delta_pos = anims[l.anim_index]->root_motion_pos.delta(layer_cursor_prev, l.cursor);
                gfxm::vec3 delta_pos4 = gfxm::vec4(delta_pos.x, delta_pos.y, delta_pos.z, 1.0f);
                gfxm::quat delta_q = anims[l.anim_index]->root_motion_rot.delta(layer_cursor_prev, l.cursor);
                if(root_motion_transform->parentTransform()) {
                    delta_pos4 = root_motion_transform->getParentTransform() * delta_pos4;
                    gfxm::quat w_rot = root_motion_transform->worldRotation();
                }
                root_motion_pos_delta = gfxm::vec3(delta_pos4.x, delta_pos4.y, delta_pos4.z);
                root_motion_rot_delta = gfxm::inverse(root_motion_transform->rotation()) * delta_q * root_motion_transform->rotation();
            }

            switch(l.mode) {
            case Layer::BASE:
                memcpy(locals_fin.begin, locals.begin, locals_fin.size());
                if(root_motion_enabled) {
                    rm_pos_final = root_motion_pos_delta;
                    rm_rot_final = root_motion_rot_delta;
                }
                break;
            case Layer::BLEND: 
                {
                    ozz::Range<ozz::math::SoaTransform> locals_blend = 
                        ozz::memory::default_allocator()->AllocateRange<ozz::math::SoaTransform>(skeleton->num_soa_joints());
                
                    ozz::animation::BlendingJob::Layer layers[2];
                    layers[0].transform = locals_fin;
                    layers[0].weight = 1.0f - l.weight;
                    layers[1].transform = locals;
                    layers[1].weight = l.weight;
                    ozz::animation::BlendingJob blend_job;
                    blend_job.threshold = 1.0f;
                    blend_job.layers = layers;
                    blend_job.bind_pose = skeleton->bind_pose();
                    blend_job.output = locals_blend;

                    // Blends.
                    if (!blend_job.Run()) {
                        LOG_WARN("Blending job failed");
                        return;
                    }
                    memcpy(locals_fin.begin, locals_blend.begin, locals_fin.size());
                    ozz::memory::default_allocator()->Deallocate(locals_blend);

                    if(root_motion_enabled) {
                        rm_pos_final = gfxm::lerp(rm_pos_final, root_motion_pos_delta, l.weight);
                        rm_rot_final = gfxm::slerp(rm_rot_final, root_motion_rot_delta, l.weight);
                    }
                }
                break;
            case Layer::ADD:
                {
                    ozz::Range<ozz::math::SoaTransform> locals_blend = 
                        ozz::memory::default_allocator()->AllocateRange<ozz::math::SoaTransform>(skeleton->num_soa_joints());
                
                    ozz::animation::BlendingJob::Layer layers[1];
                    layers[0].transform = locals_fin;
                    layers[0].weight = 1.0f;
                    ozz::animation::BlendingJob::Layer layers_add[1];
                    layers_add[0].transform = locals;
                    layers_add[0].weight = l.weight;

                    ozz::animation::BlendingJob blend_job;
                    blend_job.threshold = 1.0f;
                    blend_job.layers = layers;
                    blend_job.additive_layers = layers_add;
                    blend_job.bind_pose = skeleton->bind_pose();
                    blend_job.output = locals_blend;

                    // Blends.
                    if (!blend_job.Run()) {
                        LOG_WARN("Blending job failed");
                        return;
                    }
                    memcpy(locals_fin.begin, locals_blend.begin, locals_fin.size());
                    ozz::memory::default_allocator()->Deallocate(locals_blend);

                    // NOTE: Additive root motion is untested
                    if(root_motion_enabled) {
                        rm_pos_final = gfxm::lerp(rm_pos_final, rm_pos_final + root_motion_pos_delta, l.weight);
                        rm_rot_final = gfxm::slerp(rm_rot_final, root_motion_rot_delta * rm_rot_final, l.weight);
                    }
                }
                break;
            };
            
            ozz::memory::default_allocator()->Deallocate(locals);    
        }

        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = skeleton;
        ltm_job.input = locals_fin;
        ltm_job.output = models;
        if (!ltm_job.Run()) {
            LOG_WARN("Local to model job failed");
            return;
        }

        auto names = skeleton->joint_names();
        for(size_t i = 0; i < names.size() / sizeof(char*); ++i) {
            auto so = getObject()->findObject(names[i]);
            if(!so) continue;
            so->get<Transform>()->setTransform(*(gfxm::mat4*)&models[i]);
        }

        if(root_motion_enabled) {
            Transform* t = getObject()->get<Transform>();
            
            t->translate(rm_pos_final);
            t->rotate(rm_rot_final);
        }

        ozz::memory::default_allocator()->Deallocate(locals_fin);
        ozz::memory::default_allocator()->Deallocate(models);
        ozz::memory::default_allocator()->Delete(cache);
    }

    size_t selected_index = 0;
    virtual void _editorGui() {
        ImGui::Checkbox("Enable root motion", &root_motion_enabled);

        ImGui::Text("Layers");
        ImGui::BeginChild("Layers", ImVec2(0, 100), false, 0);
        
        for(size_t i = 0; i < layers.size(); ++i) {
            if(ImGui::Selectable(MKSTR("Layer " << i).c_str(), selected_index == i)) {
                selected_index = i;
            }
        }
        ImGui::EndChild();
        if(ImGui::Button("Add")) {
            layers.emplace_back(Layer());
            selected_index = layers.size() - 1;
        } ImGui::SameLine(); 
        if(ImGui::Button("Remove") && !layers.empty()) {
            layers.erase(layers.begin() + selected_index);
            if(selected_index >= layers.size()) {
                selected_index = layers.size() - 1;
            }
        }
        if(!layers.empty()) {
            std::string current_anim_name = "";
            if(!anims.empty()) {
                current_anim_name = anims[layers[selected_index].anim_index]->Name();
            }

            if (ImGui::BeginCombo("Current anim", current_anim_name.c_str(), 0)) {
                for(size_t i = 0; i < anims.size(); ++i) {
                    if(ImGui::Selectable(anims[i]->Name().c_str(), layers[selected_index].anim_index == i)) {
                        layers[selected_index].anim_index = i;
                    }
                }
                ImGui::EndCombo();
            }

            if(ImGui::BeginCombo("Mode", Layer::blendModeString(layers[selected_index].mode).c_str())) {
                for(size_t i = 0; i < Layer::BLEND_MODE_LAST; ++i) {
                    if(ImGui::Selectable(Layer::blendModeString((Layer::BlendMode)i).c_str(), layers[selected_index].mode == i)) {
                        layers[selected_index].mode = (Layer::BlendMode)i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SliderFloat("Cursor", &layers[selected_index].cursor, 0.0f, 1.0f);
            if(ImGui::DragFloat("Speed", &layers[selected_index].speed, 0.01f, 0.0f, 10.0f)) {}
            if(ImGui::DragFloat("Weight", &layers[selected_index].weight, 0.01f, 0.0f, 1.0f)) {}
            bool looping = true;
            if(ImGui::Checkbox("Looping", &looping)) {
            } ImGui::SameLine();
            bool autoplay = true;
            if(ImGui::Checkbox("Autoplay", &autoplay)) {
            }
        }
        ImGui::Separator();

        for(auto a : anims) {
            bool selected = false;
            ImGui::Selectable(a->Name().c_str(), &selected);
        }
        //ImGui::ListBox("Animations", 0, anim_list.data(), anim_list.size(), 4);
    }
private:
    bool root_motion_enabled = true;
    std::vector<Layer> layers;
    std::vector<std::shared_ptr<Animation>> anims;
    std::shared_ptr<Skeleton> skeleton;
};
STATIC_RUN(Animator)
{
    rttr::registration::class_<Animator>("Animator")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
