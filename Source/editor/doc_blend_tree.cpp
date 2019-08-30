#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

#include "editor_resource_tree.hpp"

#include "../common/util/imgui_helpers.hpp"

#include "../common/util/log.hpp"

DocBlendTree::DocBlendTree() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
}

struct Node {
    std::string name;
    std::vector<std::string> ins;
    std::vector<std::string> outs;
    ImVec2 pos;
};

struct Trans {
    size_t from;
    size_t from_out;
    size_t to;
    size_t to_in;
    bool operator<(const Trans& other) const {
        return from < other.from || (!(other.from < from) && to < other.to);
    }
};

#include "../common/util/node_graph.hpp"

std::vector<std::shared_ptr<IFuncNode>> nodes_;
std::vector<ImVec2> node_poses;
static std::vector<Trans> transitions;

void printer(float value) {
    LOG("VALUE: " << value);
}
void foo(float a, float b, float& result) {
    result = a * b;
}
void hello(float& result) {
    static float s = .0f;
    s += 0.01f;
    result = s;
}
int initNodes() {
    nodes_.emplace_back(createFuncNode(hello));
    nodes_.emplace_back(createFuncNode(foo));
    nodes_.emplace_back(createFuncNode(printer));
    
    node_poses.resize(3);
    return 0;
}

STATIC_RUN(FUNC_NODES) {
  regFuncNode("foo", foo, { "a", "b", "result" });
  regFuncNode("hello", hello, { "a", "b", "result" });
}

void DocBlendTree::onGui(Editor* ed, float dt) {
    static int dummy = initNodes();
    for(auto& n : nodes_) {
        n->invoke();
    }

    transitions.clear();
    for(size_t ni = 0; ni < nodes_.size(); ++ni) {
        auto& n = nodes_[ni];
        for(size_t i = 0; i < n->inputCount(); ++i) {
            size_t src_pt_index;
            IFuncNode* src = n->getInputSource(i, src_pt_index);
            if(src) {
                for(size_t j = 0; j < nodes_.size(); ++j) {
                    if(nodes_[j].get() == src) {
                        transitions.push_back({
                            j, src_pt_index, ni, i
                        });
                    }
                }
            }
        }
    }

    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(size_t i = 0; i < nodes_.size(); ++i){
            auto& n = nodes_[i];
            auto& desc = n->getDesc();
            
            ImGuiExt::BeginTreeNode(desc.name.c_str(), &node_poses[i], ImVec2(200, 0));
            for(size_t j = 0; j < desc.ins.size(); ++j) {
                auto& in = desc.ins[j];
                size_t new_conn_node;
                size_t out_pt;
                std::string pt_name = MKSTR(in.name << " (" << in.type.get_name().to_string() << ")");
                if(ImGuiExt::TreeNodeIn(pt_name.c_str(), &new_conn_node, &out_pt)) {
                    n->connectInput(j, nodes_[new_conn_node].get(), out_pt);
                }
            }
            for(size_t j = 0; j < desc.outs.size(); ++j) {
                auto& out = desc.outs[j];
                size_t new_conn_node;
                size_t in_pt;
                std::string pt_name = MKSTR(out.name << " (" << out.type.get_name().to_string() << ")");
                if(ImGuiExt::TreeNodeOut(pt_name.c_str(), &new_conn_node, &in_pt)) {
                    nodes_[new_conn_node]->connectInput(in_pt, n.get(), j);
                }
            }
            ImGuiExt::EndTreeNode();
        }

        for(auto& t : transitions) {
            ImGuiExt::TreeNodeConnection(t.from, t.to, t.from_out, t.to_in);
        }
        /*
        for(size_t i = 0; i < nodes.size(); ++i) {
            auto& n = nodes[i];
            ImGuiExt::BeginTreeNode(n.name.c_str(), &n.pos, ImVec2(100, 0));
            for(size_t j = 0; j < n.ins.size(); ++j) {
                auto& in = n.ins[j];
                size_t conn_node;
                size_t conn_point;
                if(ImGuiExt::TreeNodeIn(in.c_str(), &conn_node, &conn_point)) {
                    transitions.insert(Trans{ conn_node, conn_point, i, j });
                }
            }
            for(size_t j = 0; j < n.outs.size(); ++j) {
                auto& out = n.outs[j];
                size_t conn_node;
                size_t conn_point;
                if(ImGuiExt::TreeNodeOut(out.c_str(), &conn_node, &conn_point)) {
                    transitions.insert(Trans{ i, j, conn_node, conn_point });
                }
            }
            ImGuiExt::EndTreeNode();
        }

        ImVec2 pos(600, 0);
        ImGuiExt::BeginTreeNode("Result", &pos, ImVec2(100, 0));
        if(ImGuiExt::TreeNodeIn("pose")) {
            
        }
        ImGuiExt::EndTreeNode();

        for(auto& t : transitions) {
            ImGuiExt::TreeNodeConnection(t.from, t.to, t.from_out, t.to_in);
        }*/
    }
    ImGuiExt::EndGridView();
    ImGui::NextColumn();

    if(cam_pivot) {
        viewport.camSetPivot(cam_pivot->getTransform()->getWorldPosition());
    }
    auto anim = retrieve<Animation>("model/test/anim/rig_walk.anm");
    static float cursor = .0f;
    std::vector<AnimSample> samples;
    std::vector<ktNode*> tgt_nodes;
    if(anim && skel) {
        tgt_nodes.resize(skel->boneCount());
        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        samples.resize(skel->boneCount());
        std::vector<int32_t> mapping;
        buildAnimSkeletonMapping(anim.get(), skel.get(), mapping);

        anim->sample_remapped(samples, cursor, mapping);
        for(size_t i = 0; i < samples.size(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(samples[i].t);
                n->getTransform()->setRotation(samples[i].r);
                n->getTransform()->setScale(samples[i].s);
            }
        }
        cursor += dt * anim->fps;
        if(cursor > anim->length) {
            cursor -= anim->length;
        }
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
    if(ImGui::Button(ICON_MDI_PLUS " Blend node")) {

    }
    if(ImGui::Button(ICON_MDI_PLUS " Anim clip")) {
        clips.emplace_back(std::shared_ptr<Animation>());
    }
    if(ImGui::Button(ICON_MDI_PLUS " Parameter")) {
        
    }

    for(size_t i = 0; i < clips.size(); ++i) {
        auto& c = clips[i];
        std::string label = MKSTR("###" << i);
        imguiResourceTreeCombo(label.c_str(), c, "anm", [](){});
    }

    imguiResourceTreeCombo("reference", ref_scn, "so", [](){

    });
    imguiResourceTreeCombo("skeleton", skel, "skl", [](){

    });
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });
}