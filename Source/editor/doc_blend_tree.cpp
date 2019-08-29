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

static std::vector<Node> nodes;
static std::set<Trans> transitions;

int initNodes() {
    nodes.emplace_back(
        Node{
            "test",
            { "r", "g", "b", "a" },
            { "out" },
            ImVec2(0,0)
        }
    );
    nodes.emplace_back(
        Node{
            "blend",
            { "fuck", "this", "shit", "I'm" },
            { "out" },
            ImVec2(300,0)
        }
    );
    return 0;
}

void DocBlendTree::onGui(Editor* ed, float dt) {
    static int dummy = initNodes();

    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
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
        }
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