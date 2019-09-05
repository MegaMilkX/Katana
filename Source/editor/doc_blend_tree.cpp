#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

#include "editor_resource_tree.hpp"

#include "../common/util/imgui_helpers.hpp"

#include "../common/util/log.hpp"

DocBlendTree::DocBlendTree() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);
}

void blend2(const BlendSeq& a, const BlendSeq& b, float weight, BlendSeq& blend_seq) {
    blend_seq.seq.clear();
    blend_seq.seq.insert(blend_seq.seq.end(), a.seq.begin(), a.seq.end());
    for(auto item : b.seq) {
        item.weight *= weight;
        blend_seq.seq.emplace_back(item);
    }
}

void blend3(
    const BlendSeq& a,
    const BlendSeq& b,
    const BlendSeq& c,
    float weight,
    BlendSeq& blend_seq
) {
    blend_seq.seq.clear();
    if(weight < .0f) {
        weight = .0f;
    }
    if(weight > 1.0f) {
        weight = 1.0f;
    }
    const BlendSeq* array[] = {
        &a, &b, &c
    };

    weight *= 2.0f;

    int left_idx = weight;
    if(left_idx == 2) {
        for(auto item : c.seq) {
            item.weight;
            blend_seq.seq.emplace_back(item);
        }
        return;
    }
    int right_idx = left_idx + 1;
    float lr_weight = weight - (float)left_idx;

    blend2(*array[left_idx], *array[right_idx], lr_weight, blend_seq);
}


STATIC_RUN(FUNC_NODES) {
    regJobNode<TestJob>("Test")
        .out<float>("value");
    regJobNode<MultiplyJob>("Multiply")
        .in<float>("a")
        .in<float>("b")
        .out<float>("result");
    regJobNode<PrintJob>("Print")
        .in<float>("result");
}

void guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
    bool clicked = false;
    bool selected = false;
    ImGuiExt::BeginTreeNode(MKSTR(node->getDesc().name).c_str(), pos, &clicked, selected, ImVec2(200, 0));

    void* new_conn_tgt = 0;
    for(size_t j = 0; j < node->getDesc().ins.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeIn(node->getDesc().ins[j].name.c_str(), (void*)node->getInput(j), (void*)node->getInput(j)->source)) {
            node->connect(j, (JobOutput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }
    for(size_t j = 0; j < node->getDesc().outs.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeOut(node->getDesc().outs[j].name.c_str(), (void*)node->getOutput(j), 0)) {
            node->connect(j, (JobInput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }

    ImGuiExt::EndTreeNode();
}
std::map<JobGraphNode*, ImVec2> node_poses;
void DocBlendTree::onGui(Editor* ed, float dt) {
    jobGraph.run();

    static IBaseNode* selected_graph_node = 0;
    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(auto n : jobGraph.getNodes()) {
            guiDrawNode(jobGraph, n, &node_poses[n]);
        }
    }
    ImGuiExt::EndGridView();
    ImGui::NextColumn();

    
/*
    static float cursor = .0f;
    std::vector<AnimSample> samples;
    std::vector<ktNode*> tgt_nodes;
    if(skel) {
        tgt_nodes.resize(skel->boneCount());
        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        samples.resize(skel->boneCount());

        BlendSeq& seq = result_node->get();
        for (auto& item : seq.seq) {
          Animation* anim = item.anim;
          float weight = item.weight;
          if (anim == 0) {
            continue;
          }
          anim->blend_remapped(samples, cursor * anim->length, weight, item.mapping);
        }
        
        for(size_t i = 0; i < samples.size(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(samples[i].t);
                n->getTransform()->setRotation(samples[i].r);
                n->getTransform()->setScale(samples[i].s);
            }
        }
        cursor += dt;
        if(cursor > 1.0f) {
            cursor -= 1.0f;
        }
    }*/

    if(cam_pivot) {
        viewport.camSetPivot(cam_pivot->getTransform()->getWorldPosition());
    }
    viewport.draw(&scn);
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_DND_RESOURCE)) {
            ResourceNode* node = *(ResourceNode**)payload->Data;
            LOG("Payload received: " << node->getFullName());
            if(has_suffix(node->getName(), ".so")) {
                cam_pivot = 0;
                
                auto ref = node->getResource<GameScene>();
                if(ref) {
                    ref_scn = ref;

                    scn.clear();
                    scn.copy(node->getResource<GameScene>().get());
                    scn.getTransform()->setScale(
                        node->getResource<GameScene>()->getTransform()->getScale()
                    );
                    gfxm::aabb box;
                    scn.makeAabb(box);
                    viewport.resetCamera((box.from + box.to) * 0.5f, gfxm::length(box.to - box.from));
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndColumns();
}
void DocBlendTree::onGuiToolbox(Editor* ed) {
    ImGui::BeginGroup();
    
    ImGui::EndGroup();
    
    if(ImGui::Button(ICON_MDI_PLUS " Add node")) {
        ImGui::OpenPopup("test");
    }
    if(ImGui::BeginPopup("test")) {
        if(ImGui::MenuItem("Multiply")) {
            jobGraph.addNode(new MultiplyJob);
            jobGraph.prepare();
        }
        if(ImGui::MenuItem("Printer")) {
            jobGraph.addNode(new PrintJob);
            jobGraph.prepare();
        }
        if(ImGui::MenuItem("Test")) {
            jobGraph.addNode(new TestJob);
            jobGraph.prepare();
        }
        ImGui::EndPopup();
    }
/*
    for(size_t i = 0; i < clips.size(); ++i) {
        auto& c = clips[i];
        std::string label = MKSTR("###" << i);
        imguiResourceTreeCombo(label.c_str(), c, "anm", [this, &i](){
            if(clips[i]) {
                std::vector<int32_t> mapping;
                if(skel) {
                    buildAnimSkeletonMapping(clips[i].get(), skel.get(), mapping);
                }
                clip_nodes[i]->getDesc().name = clips[i]->Name();
                clip_nodes[i]->set(BlendSeq{{{ clips[i].get(), 1.0f, mapping }}});
            } else {
                clip_nodes[i]->getDesc().name = "Empty";
                clip_nodes[i]->set(BlendSeq{{{ 0, 1.0f }}});
            }
        });
    }

    imguiResourceTreeCombo("reference", ref_scn, "so", [](){

    });
    imguiResourceTreeCombo("skeleton", skel, "skl", [this](){
        if(!skel) { return; }
        for(size_t i = 0; i < clip_nodes.size(); ++i) {
            auto& mapping = clip_nodes[i]->get().seq.back().mapping;
            buildAnimSkeletonMapping(clips[i].get(), skel.get(), mapping);
        }
    });
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });

    for(auto n : weight_nodes) {
        ImGui::DragFloat(MKSTR(n->getDesc().name << "###" << n).c_str(), &n->get(), 0.001f, 0.0f, 1.0f);
    }*/
}