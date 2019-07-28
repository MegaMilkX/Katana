#ifndef ANIMATION_STACK_HPP
#define ANIMATION_STACK_HPP

#include "component.hpp"

#include "../../common/resource/animation.hpp"
#include "../../common/resource/skeleton.hpp"

#include "../../common/resource/resource_tree.hpp"

#include "../transform.hpp"

enum ANIM_BLEND_MODE {
    ANIM_MODE_NONE,
    ANIM_MODE_BLEND,
    ANIM_MODE_ADD,
    ANIM_MODE_LAST
};
inline std::string blendModeString(ANIM_BLEND_MODE m) {
    std::string r = "UNKNOWN";
    switch(m) {
        case ANIM_MODE_NONE: r = "BASE"; break;
        case ANIM_MODE_BLEND: r = "BLEND"; break;
        case ANIM_MODE_ADD: r = "ADD"; break;
    }
    return r;
}

class AnimLayer {
public:
    int             anim_index = 0;
    ANIM_BLEND_MODE mode = ANIM_MODE_NONE;
    float           cursor = 0.0f;
    float           speed = 1.0f;
    float           weight = 0.0f;

    int             blend_target_index = 0;
    float           blend_target_cursor = 0;
    float           blend_target_prev_cursor = 0;
    float           blend_over_weight = 0;
    float           blend_over_speed = 0;
    
    bool            stopped = false;
};

class AnimationStack : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    struct AnimInfo {
        std::string                 alias;
        std::shared_ptr<Animation>  anim;
        std::vector<size_t>         bone_remap;
        bool                        looping = true;
    };

    AnimationStack() {
        auto& layer0 = addLayer();
        layer0.anim_index = 0;
        layer0.weight = 1.0f;
    }
    ~AnimationStack();

    virtual void onCreate();

    void play(int layer, const std::string& anim_alias);
    void blendOver(int layer, const std::string& anim_alias, float speed = 0.1f);
    void blendOverFromCurrentPose(float blend_time = 0.1f);
    bool layerStopped(int layer, float offset = 0.0f);
    float getLengthProportion(int layer_a, int layer_b);

    virtual void copy(Attribute* other);

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    void addAnim(std::shared_ptr<Animation> anim);
    void reserveLayers(unsigned count);
    AnimLayer& addLayer();
    AnimLayer& getLayer(unsigned i);
    void update(float dt);

    void setEventCallback(const std::string& name, std::function<void(void)> cb);
    float getCurveValue(const std::string& name);

    virtual bool serialize(out_stream& out);
    virtual bool deserialize(in_stream& in, size_t sz);

    void onGui() {
        auto anim_stack = this;
        static int selected_index = 0;
        static int selected_anim_index = 0;

        if(selected_anim_index >= anims.size()) {
            selected_anim_index = anims.size() - 1;
            if(selected_anim_index < 0) {
                selected_anim_index = 0;
            }
        }

        ImGui::Text("Skeleton "); ImGui::SameLine();
        std::string button_label = "! No skeleton !";
        if(anim_stack->skeleton) {
            button_label = anim_stack->skeleton->Name();
        }
        if(ImGui::SmallButton(button_label.c_str())) {

        }
        ImGui::PushID(button_label.c_str());
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                anim_stack->setSkeleton(retrieve<Skeleton>(fname));
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();

        

        ImGui::Checkbox("Enable root motion", &anim_stack->root_motion_enabled);

        ImGui::Text("Layers");
        ImGui::BeginChild("Layers", ImVec2(0, 100), false, 0);
        
        for(size_t i = 0; i < anim_stack->layers.size(); ++i) {
            if(ImGui::Selectable(MKSTR("Layer " << i).c_str(), selected_index == i)) {
                selected_index = i;
            }
        }
        ImGui::EndChild();
        if(ImGui::Button("Add")) {
            anim_stack->addLayer();
            selected_index = anim_stack->layers.size() - 1;
        } ImGui::SameLine(); 
        if(ImGui::Button("Remove") && !anim_stack->layers.empty()) {
            anim_stack->layers.erase(anim_stack->layers.begin() + selected_index);
            if(selected_index >= anim_stack->layers.size()) {
                selected_index = anim_stack->layers.size() - 1;
            }
        }
        if(!anim_stack->layers.empty()) {
            std::string current_anim_name = "";
            if(!anim_stack->anims.empty()) {
                current_anim_name = anim_stack->anims[anim_stack->layers[selected_index].anim_index].alias;
            }

            if (ImGui::BeginCombo("Current anim", current_anim_name.c_str(), 0)) {
                for(size_t i = 0; i < anim_stack->anims.size(); ++i) {
                    if(ImGui::Selectable(anim_stack->anims[i].alias.c_str(), anim_stack->layers[selected_index].anim_index == i)) {
                        anim_stack->layers[selected_index].anim_index = i;
                    }
                }
                ImGui::EndCombo();
            }

            if(ImGui::BeginCombo("Mode", blendModeString(anim_stack->layers[selected_index].mode).c_str())) {
                for(size_t i = 0; i < ANIM_MODE_LAST; ++i) {
                    if(ImGui::Selectable(blendModeString((ANIM_BLEND_MODE)i).c_str(), anim_stack->layers[selected_index].mode == i)) {
                        anim_stack->layers[selected_index].mode = (ANIM_BLEND_MODE)i;
                    }
                }
                ImGui::EndCombo();
            }
            float anim_len = 1.0f;
            if(anim_stack->anims.size() > anim_stack->layers[selected_index].anim_index) {
                anim_len = anim_stack->anims[anim_stack->layers[selected_index].anim_index].anim->length;
            }
            ImGui::SliderFloat(
                "Cursor", 
                &anim_stack->layers[selected_index].cursor, 
                0.0f, 
                anim_len
            );
            if(ImGui::DragFloat("Speed", &anim_stack->layers[selected_index].speed, 0.01f, 0.0f, 10.0f)) {}
            if(ImGui::DragFloat("Weight", &anim_stack->layers[selected_index].weight, 0.01f, 0.0f, 1.0f)) {}
            if(ImGui::Checkbox("Stopped", &anim_stack->layers[selected_index].stopped)) {
            }
        }
        ImGui::Separator();

        ImGui::BeginChild("Animations", ImVec2(0, 100), false, 0);
        for(size_t i = 0; i < anim_stack->anims.size(); ++i) {
            if(ImGui::Selectable(anim_stack->anims[i].alias.c_str(), selected_anim_index == i)) {
                selected_anim_index = i;
            }
        }
        ImGui::EndChild();
        ImGui::PushID("Animations");
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                auto anim = retrieve<Animation>(fname);
                anim_stack->addAnim(anim);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();

        if(!anim_stack->anims.empty()) {
            std::string current_anim_name = "";
            if(!anim_stack->anims.empty()) {
                current_anim_name = anim_stack->anims[selected_anim_index].alias;
            }
            char buf[256];
            memcpy(buf, anim_stack->anims[selected_anim_index].alias.data(), anim_stack->anims[selected_anim_index].alias.size());
            if(ImGui::InputText("Alias", buf, 256)) {
                anim_stack->anims[selected_anim_index].alias = buf;
            }
            if(ImGui::Checkbox("Looping", &anim_stack->anims[selected_anim_index].looping)) {
            }
            if(ImGui::Button("Remove anim")) {
                anim_stack->anims.erase(anim_stack->anims.begin() + selected_anim_index);
                for(auto l : anim_stack->layers) {
                    if(l.anim_index == selected_anim_index) {
                        l.anim_index = 0;
                    }
                    selected_anim_index = 0;
                }
            }
        }
    }

    virtual const char* getIconCode() const { return ICON_MDI_LAYERS; }
private:
    void updateLayer(
        AnimLayer& l, 
        float dt,
        gfxm::vec3& rm_pos_final,
        gfxm::quat& rm_rot_final
    );
    void blendAnim(
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
        std::function<void(const std::string&)> evt_callback = nullptr,
        float event_threshold = 0.0f
    );

    void resetSkeletonMapping();
    void resetSkeletonMapping(AnimInfo& anim_info);
    void resetSampleBuffer();

    std::shared_ptr<Skeleton> skeleton;
    std::vector<AnimInfo>     anims;
    std::vector<AnimLayer>    layers;
    std::vector<AnimSample>   samples;
    std::vector<AnimSample>   blend_over_origin_samples;
    float                     blend_over_speed = .0f;
    float                     blend_over_weight = .0f;

    std::map<std::string, float> curve_values;
    std::map<std::string, std::function<void(void)>> callbacks;

    bool root_motion_enabled = true;
};
STATIC_RUN(AnimationStack) {
    rttr::registration::class_<AnimationStack>("AnimationStack")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
};

#endif
