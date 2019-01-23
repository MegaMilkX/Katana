#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "component.hpp"
#include "resource/animation.hpp"
#include "resource/skeleton.hpp"
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/maths/soa_transform.h>


class Animator : public Component {
    //CLONEABLE
public:
    std::shared_ptr<Skeleton> skeletom_;

    ozz::animation::Skeleton* skeleton = 0;

    Animator() {
        auto& layer0 = addLayer();
        layer0.anim_index = 0;
    }

    ~Animator() {
        if(skeleton)
            ozz::memory::default_allocator()->Delete(skeleton);
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
        ozz::animation::SamplingCache* cache = 
            ozz::memory::default_allocator()->New<ozz::animation::SamplingCache>(skeleton->num_joints());
        ozz::Range<ozz::math::SoaTransform> locals_fin = 
            ozz::memory::default_allocator()->AllocateRange<ozz::math::SoaTransform>(skeleton->num_soa_joints());
        ozz::Range<ozz::math::Float4x4> models =
            ozz::memory::default_allocator()->AllocateRange<ozz::math::Float4x4>(skeleton->num_joints());
        
        for(auto& l : layers) {
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
                root_motion_transform = 
                    getObject()->findObject(anims[l.anim_index]->root_motion_node)->get<Transform>();
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

            switch(l.mode) {
            case Layer::BASE:
                memcpy(locals_fin.begin, locals.begin, locals_fin.size());
                if(anims[l.anim_index]->root_motion_enabled && root_motion_transform) {
                    gfxm::vec3 delta_pos = anims[l.anim_index]->root_motion_pos.delta(layer_cursor_prev, l.cursor);
                    gfxm::vec3 delta_pos4 = gfxm::vec4(delta_pos.x, delta_pos.y, delta_pos.z, 1.0f);
                    gfxm::quat delta_q = anims[l.anim_index]->root_motion_rot.delta(layer_cursor_prev, l.cursor);
                    if(root_motion_transform->parentTransform()) {
                        delta_pos4 = root_motion_transform->getParentTransform() * delta_pos4;
                        gfxm::quat w_rot = root_motion_transform->worldRotation();
                        delta_q = delta_q * gfxm::inverse(w_rot);
                    }
                    delta_pos = gfxm::vec3(delta_pos4.x, delta_pos4.y, delta_pos4.z);
                    Transform* t = getObject()->get<Transform>();
                    t->translate(delta_pos);
                    t->rotate(delta_q);
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

        ozz::memory::default_allocator()->Deallocate(locals_fin);
        ozz::memory::default_allocator()->Deallocate(models);
        ozz::memory::default_allocator()->Delete(cache);
    }

    size_t selected_index = 0;
    virtual void _editorGui() {
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
            if (ImGui::BeginCombo("Current anim", anims[layers[selected_index].anim_index]->Name().c_str(), 0)) {
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
    std::vector<Layer> layers;
    std::vector<std::shared_ptr<Animation>> anims;
};
STATIC_RUN(Animator)
{
    rttr::registration::class_<Animator>("Animator")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
