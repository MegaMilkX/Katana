#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "component.hpp"
#include "scene_object.hpp"
#include "transform.hpp"

#include "resource/animation.hpp"
#include "resource/skeleton.hpp"
#include "resource/resource_factory.h"

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

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        updateTransformBuffer();
        updateAnimationMapping();
    }

    void addAnim(std::shared_ptr<Animation> anim) {
        anims.emplace_back(
            AnimInfo {
                anim->Name().substr(anim->Name().find_last_of("/")),
                anim
            }
        );
        updateAnimationMapping(anims.back());
    }

    SkeletonAnimLayer& addLayer() {
        layers.emplace_back(SkeletonAnimLayer());
        return layers.back();
    }

private:
    struct AnimInfo {
        std::string                 alias;
        std::shared_ptr<Animation>  anim;
        std::map<size_t, size_t>    bone_remap;
    };

    std::vector<AnimInfo>       anims;
    std::shared_ptr<Skeleton>   skeleton;
    std::vector<AnimSample>     sample_buffer;
    std::vector<gfxm::mat4>     bone_transforms;
    std::vector<std::string>    bone_names;

public:    
    void Update(float dt) {
        if(!skeleton) {
            return;
        }
        gfxm::vec3 rm_pos_final = gfxm::vec3(0.0f, 0.0f, 0.0f);
        gfxm::quat rm_rot_final = gfxm::quat(0.0f, 0.0f, 0.0f, 1.0f);

        for(auto& l : layers) {
            l.update(
                this, dt, skeleton.get(),
                sample_buffer,
                rm_pos_final, rm_rot_final
            );
        }

        for(size_t i = 0; i < sample_buffer.size(); ++i) {
            auto& s = sample_buffer[i];
            auto& m = bone_transforms[i];
            m = gfxm::translate(gfxm::mat4(1.0f), s.t) * 
                gfxm::to_mat4(s.r) * 
                gfxm::scale(gfxm::mat4(1.0f), s.s);
        }

        // TODO: Optimize
        for(size_t i = 0; i < skeleton->boneCount(); ++i) {
            Skeleton::Bone& b = skeleton->getBone(i);
            auto so = getObject()->findObject(b.name);
            if(!so) continue;
            so->get<Transform>()->setTransform(bone_transforms[i]);
        }

        // Update root motion target
        if(root_motion_enabled) {
            Transform* t = getObject()->get<Transform>();
            
            t->translate(rm_pos_final);
            t->rotate(rm_rot_final);
        }
    }

    size_t selected_index = 0;
    virtual void _editorGui() {
        ImGui::Text("Skeleton "); ImGui::SameLine();
        std::string button_label = "! No skeleton !";
        if(skeleton) {
            button_label = skeleton->Name();
        }
        if(ImGui::SmallButton(button_label.c_str())) {

        }
        ImGui::PushID(button_label.c_str());
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                setSkeleton(getResource<Skeleton>(fname));
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();

        

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
            addLayer();
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
                current_anim_name = anims[layers[selected_index].anim_index].anim->Name();
            }

            if (ImGui::BeginCombo("Current anim", current_anim_name.c_str(), 0)) {
                for(size_t i = 0; i < anims.size(); ++i) {
                    if(ImGui::Selectable(anims[i].alias.c_str(), layers[selected_index].anim_index == i)) {
                        layers[selected_index].anim_index = i;
                    }
                }
                ImGui::EndCombo();
            }

            if(ImGui::BeginCombo("Mode", SkeletonAnimLayer::blendModeString(layers[selected_index].mode).c_str())) {
                for(size_t i = 0; i < SkeletonAnimLayer::BLEND_MODE_LAST; ++i) {
                    if(ImGui::Selectable(SkeletonAnimLayer::blendModeString((SkeletonAnimLayer::BlendMode)i).c_str(), layers[selected_index].mode == i)) {
                        layers[selected_index].mode = (SkeletonAnimLayer::BlendMode)i;
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

        ImGui::BeginChild("Animations", ImVec2(0, 100), false, 0);
        for(auto a : anims) {
            bool selected = false;
            ImGui::Selectable(a.alias.c_str(), &selected);
        }
        ImGui::EndChild();
        ImGui::PushID("Animations");
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                auto anim = getResource<Animation>(fname);
                addAnim(anim);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
        //ImGui::ListBox("Animations", 0, anim_list.data(), anim_list.size(), 4);
    }
private:
    bool root_motion_enabled = true;
    std::vector<SkeletonAnimLayer> layers;

    void updateAnimationMapping() {
        if(!skeleton) return;
        for(auto& a : anims) {
            updateAnimationMapping(a);
        }
    }
    void updateAnimationMapping(AnimInfo& anim_info) {
        if(!skeleton) return;

        for(size_t i = 0; i < skeleton->boneCount(); ++i) {
            Skeleton::Bone& b = skeleton->getBone(i);
            int32_t bone_index = (int32_t)i;
            int32_t node_index = anim_info.anim->getNodeIndex(b.name);
            if(node_index >= 0) {
                anim_info.bone_remap[node_index] = bone_index;
            }
        }
    }
    void updateTransformBuffer() {
        if(!skeleton) return;

        bone_transforms.resize(skeleton->boneCount());
        sample_buffer.resize(skeleton->boneCount());
        for(size_t i = 0; i < skeleton->boneCount(); ++i) {
            Skeleton::Bone& b = skeleton->getBone(i);
            bone_transforms[i] = b.bind_pose;
        }
    }
};
STATIC_RUN(Animator)
{
    rttr::registration::class_<Animator>("Animator")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
