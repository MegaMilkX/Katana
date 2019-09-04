#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

#include "editor_resource_tree.hpp"

#include "../common/util/imgui_helpers.hpp"

#include "../common/util/log.hpp"

DocBlendTree::DocBlendTree() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);

    result_node = funcGraph.addResultNode<BlendSeq>("Result");
}

struct Trans {
    size_t from;
    size_t from_out;
    size_t to;
    size_t to_in;
    bool operator<(const Trans& other) const {
        return from < other.from || (!(other.from < from) && to < other.to);
    }
};

static std::vector<Trans> transitions;

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
    regFuncNode("BlendTree/blend2", blend2, { "anim0", "anim1", "weight", "out" });
    regFuncNode("BlendTree/blend3", blend3, { "0", "1", "2", "weight", "out" });
  //regFuncNode("foo", foo, { "a", "b", "result" });
  //regFuncNode("hello", hello, { "a", "b", "result" });
  //regFuncNode("printer", printer, { "input" });

  //regFuncNode("BlendTree/blend3", blend3, {"motion0", "motion1", "motion2", "weight", "out"});

  //regFuncNode("Test/foo", &TestClass::foo);
}


void DocBlendTree::onGui(Editor* ed, float dt) {
    funcGraph.run();

    transitions.clear();
    for(size_t ni = 0; ni < funcGraph.nodeCount(); ++ni) {
        auto n = funcGraph.getNode(ni);
        for(size_t i = 0; i < n->inputCount(); ++i) {
            size_t src_pt_index;
            IBaseNode* src = n->getInputSource(i, src_pt_index);
            if(src) {
                for(size_t j = 0; j < funcGraph.nodeCount(); ++j) {
                    if(funcGraph.getNode(j) == src) {
                        transitions.push_back({
                            j, src_pt_index, ni, i
                        });
                    }
                }
            }
        }
    }

    MultiplyJob multJob;
    multJob.init();

    static IBaseNode* selected_graph_node = 0;
    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        JobNode& node = multJob;
        ImVec2 pos(0,0);
        bool clicked = false;
        bool selected = false;
        ImGuiExt::BeginTreeNode(node.getDesc().name.c_str(), &pos, &clicked, selected, ImVec2(200, 0));

        for(size_t j = 0; j < node.getDesc().ins.size(); ++j) {
            if(ImGuiExt::TreeNodeIn(node.getDesc().ins[j].name.c_str())) {
            }
        }
        for(size_t j = 0; j < node.getDesc().outs.size(); ++j) {
            if(ImGuiExt::TreeNodeOut(node.getDesc().outs[j].name.c_str())) {
            }
        }

        ImGuiExt::EndTreeNode();

        /*
        for(size_t i = 0; i < funcGraph.nodeCount(); ++i){
            auto n = funcGraph.getNode(i);
            auto& desc = n->getDesc();
            
            bool clicked = false;
            bool selected = selected_graph_node == n;
            ImGuiExt::BeginTreeNode(desc.name.c_str(), &funcGraph.getNodePoses()[n], &clicked, selected, ImVec2(200, 0));
            for(size_t j = 0; j < desc.ins.size(); ++j) {
                auto& in = desc.ins[j];
                size_t new_conn_node;
                size_t out_pt;
                std::string pt_name = MKSTR(in.name << " (" << in.type.get_name().to_string() << ")");
                if(ImGuiExt::TreeNodeIn(pt_name.c_str(), &new_conn_node, &out_pt)) {
                    funcGraph.connect(
                        funcGraph.getNode(new_conn_node),
                        n,
                        out_pt,
                        j
                    );
                }
            }
            for(size_t j = 0; j < desc.outs.size(); ++j) {
                auto& out = desc.outs[j];
                size_t new_conn_node;
                size_t in_pt;
                std::string pt_name = MKSTR(out.name << " (" << out.type.get_name().to_string() << ")");
                if(ImGuiExt::TreeNodeOut(pt_name.c_str(), &new_conn_node, &in_pt)) {
                    funcGraph.connect(
                        n,
                        funcGraph.getNode(new_conn_node),
                        j,
                        in_pt
                    );
                }
            }
            ImGuiExt::EndTreeNode();
            if(clicked) {
                selected_graph_node = n;
            }
        }

        for(auto& t : transitions) {
            ImGuiExt::TreeNodeConnection(t.from, t.to, t.from_out, t.to_in);
        }*/
    }
    ImGuiExt::EndGridView();
    ImGui::NextColumn();

    

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
    }

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
    if(ImGui::Button(ICON_MDI_PLUS " Float node")) {
        weight_nodes.emplace_back(funcGraph.addDataNode<float>("test"));
    }
    if(ImGui::Button(ICON_MDI_PLUS " Blend2 node")) {
        funcGraph.addNode("BlendTree/blend2");
    }
    if(ImGui::Button(ICON_MDI_PLUS " Blend3 node")) {
        funcGraph.addNode("BlendTree/blend3");
    }
    if(ImGui::Button(ICON_MDI_PLUS " Anim clip")) {
        clip_nodes.emplace_back(funcGraph.addDataNode<BlendSeq>("AnimClip"));
        clip_nodes.back()->set(BlendSeq{{{ 0, 1.0f }}});
        clips.emplace_back(std::shared_ptr<Animation>());
    }
    if(ImGui::Button(ICON_MDI_PLUS " Parameter")) {
        
    }

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
    }
}