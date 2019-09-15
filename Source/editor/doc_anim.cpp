#include "doc_anim.hpp"

#include "../common/util/imgui_helpers.hpp"


void DocAnim::onGui(Editor* ed, float dt) {

    std::vector<ktNode*> tgt_nodes;
    if(ref_skel) {
        auto& skel = ref_skel;

        std::vector<AnimSample> pose;
        pose.resize(skel->boneCount());

        _resource->sample_remapped(pose, cursor, _resource->getMapping(skel.get()));

        tgt_nodes.resize(skel->boneCount());
        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        for(size_t i = 0; i < pose.size(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(pose[i].t);
                n->getTransform()->setRotation(pose[i].r);
                n->getTransform()->setScale(pose[i].s);
            }
        }
    }

    auto window = ImGui::GetCurrentWindow();
    auto rect = window->ContentsRegionRect;

    ImGuiContext* ctx = ImGui::GetCurrentContext();
    const ImGuiStyle& style = ctx->Style;
    float button_height = style.FramePadding.y * 2.0f;
    button_height += ImGui::GetTextLineHeightWithSpacing();
    float timeline_height = 50.0f;
    float viewport_height = (rect.Max.y - rect.Max.y) - timeline_height - button_height;

    ImGui::BeginColumns("First", 2);

    gvp.draw(&scn, 0, gfxm::ivec2(0, viewport_height));

    ImGui::NextColumn();



    ImGui::EndColumns();

    

    ImGui::Button(ICON_MDI_SKIP_BACKWARD);
    ImGui::SameLine();
    ImGui::Button(ICON_MDI_PLAY);
    ImGui::SameLine();
    ImGui::Button(ICON_MDI_FAST_FORWARD);
    ImGui::SameLine();
    ImGui::Text(MKSTR(cursor).c_str());
    
    if(ImGuiExt::BeginTimeline(_resource->length, &cursor)) {
        ImGuiExt::TimelineEvent("event_test", 11);
        ImGuiExt::TimelineMarker("marker", 16);
        ImGuiExt::EndTimeline();
    }
}  

void DocAnim::onGuiToolbox(Editor* ed) {
    imguiResourceTreeCombo("reference", ref_object, "so", [this](){
        if(ref_object) {
            scn.copy(ref_object.get());
        }
    });
    imguiResourceTreeCombo("skeleton", ref_skel, "skl", [this](){
        
    });
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });
}