#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "component.hpp"
#include "scene_object.hpp"
#include "transform.hpp"

#include "resource/animation.hpp"
#include "resource/skeleton.hpp"
#include "resource/resource_factory.h"

#include "skeleton_anim_layer.hpp"

#include "util/serialization.hpp"

class Animator : public Component {
    CLONEABLE_AUTO
    friend SkeletonAnimLayer;
public:
    Animator() {
        auto& layer0 = addLayer();
        layer0.anim_index = 0;
        layer0.weight = 1.0f;
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

    void reserveLayers(unsigned count) {
        layers.clear();
        layers.resize(count);
    }

    SkeletonAnimLayer& addLayer() {
        layers.emplace_back(SkeletonAnimLayer());
        return layers.back();
    }

    SkeletonAnimLayer& getLayer(unsigned i) {
        return layers[i];
    }

private:
    struct AnimInfo {
        std::string                 alias;
        std::shared_ptr<Animation>  anim;
        std::map<size_t, size_t>    bone_remap;
    };

    bool                            root_motion_enabled = true;
    std::vector<SkeletonAnimLayer>  layers;
    std::vector<AnimInfo>           anims;
    std::shared_ptr<Skeleton>       skeleton;

    std::vector<AnimSample>         sample_buffer;
    std::vector<gfxm::mat4>         bone_transforms;
    std::vector<std::string>        bone_names;

public:  
    void play(int layer, const std::string& anim_alias) {
        size_t anim_i = 0;
        for(size_t i = 0; i < anims.size(); ++i) {
            if(anims[i].alias == anim_alias) {
                anim_i = i;
            }
        }

        layers[layer].anim_index = anim_i;
        layers[layer].cursor = 0.0f;
    }

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

        if(layers.empty() || anims.empty()) {
            for(size_t i = 0; i < sample_buffer.size(); ++i) {
                auto& m = bone_transforms[i];
                m = skeleton->getBone(i).bind_pose;
            }
        } else {
            for(size_t i = 0; i < sample_buffer.size(); ++i) {
                auto& s = sample_buffer[i];
                auto& m = bone_transforms[i];

                m = gfxm::translate(gfxm::mat4(1.0f), s.t) * 
                    gfxm::to_mat4(s.r) * 
                    gfxm::scale(gfxm::mat4(1.0f), s.s);
            }
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

            t->rotate(rm_rot_final);

            gfxm::vec4 t4 = gfxm::vec4(
                rm_pos_final.x,
                rm_pos_final.y,
                rm_pos_final.z,
                0.0f
            );
            gfxm::mat4 root_m4 = t->getTransform();
            root_m4[3] = gfxm::vec4(.0f, .0f, .0f, 1.0f);
            root_m4[0] = gfxm::normalize(root_m4[0]);
            root_m4[1] = gfxm::normalize(root_m4[1]);
            root_m4[2] = gfxm::normalize(root_m4[2]);
            t4 = (root_m4) * t4;

            t->translate(t4);
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, root_motion_enabled);
        
        write(out, (uint32_t)layers.size());
        for(size_t i = 0; i < layers.size(); ++i) {
            SkeletonAnimLayer& l = layers[i];
            write(out, (uint32_t)l.anim_index);
            write(out, (uint32_t)l.mode);
            write(out, l.cursor);
            write(out, l.speed);
            write(out, l.weight);
        }

        write(out, (uint32_t)anims.size());
        for(size_t i = 0; i < anims.size(); ++i) {
            AnimInfo& a = anims[i];
            wt_string(out, a.alias);
            if(a.anim) {
                wt_string(out, a.anim->Name());
            } else {
                wt_string(out, "");
            }
            write(out, (uint32_t)a.bone_remap.size());
            for(auto& kv : a.bone_remap) {
                write(out, (uint32_t)kv.first);
                write(out, (uint32_t)kv.second);
            }
        }

        if(skeleton) {
            wt_string(out, skeleton->Name());
        } else {
            wt_string(out, "");
        }
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        root_motion_enabled = read<bool>(in);
        
        layers.resize(read<uint32_t>(in));
        for(size_t i = 0; i < layers.size(); ++i) {
            auto& l = layers[i];
            l.anim_index = read<uint32_t>(in);
            l.mode = (SkeletonAnimLayer::BlendMode)read<uint32_t>(in);
            l.cursor = read<float>(in);
            l.speed = read<float>(in);
            l.weight = read<float>(in);
        }

        anims.resize(read<uint32_t>(in));
        for(size_t i = 0; i < anims.size(); ++i) {
            auto& a = anims[i];
            a.alias = rd_string(in);
            a.anim = getResource<Animation>(rd_string(in));
            uint32_t bone_remap_sz = read<uint32_t>(in);
            for(uint32_t j = 0; j < bone_remap_sz; ++j) {
                auto f = read<uint32_t>(in);
                auto t = read<uint32_t>(in);
                a.bone_remap[f] = t;
            }
        }

        setSkeleton(getResource<Skeleton>(rd_string(in)));
    }

    size_t selected_index = 0;
    size_t selected_anim_index = 0;
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
        for(size_t i = 0; i < anims.size(); ++i) {
            if(ImGui::Selectable(anims[i].alias.c_str(), selected_anim_index == i)) {
                selected_anim_index = i;
            }
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

        if(!anims.empty()) {
            std::string current_anim_name = "";
            if(!anims.empty()) {
                current_anim_name = anims[selected_anim_index].alias;
            }
            char buf[256];
            memcpy(buf, anims[selected_anim_index].alias.data(), anims[selected_anim_index].alias.size());
            if(ImGui::InputText("Alias", buf, 256)) {
                anims[selected_anim_index].alias = buf;
            }
        }
    }
private:
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
