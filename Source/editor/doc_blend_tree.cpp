#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

#include "editor_resource_tree.hpp"

#include "../common/util/log.hpp"

DocBlendTree::DocBlendTree() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);
}
DocBlendTree::DocBlendTree(std::shared_ptr<ResourceNode>& node) {
    setResourceNode(node);

    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);
}

void DocBlendTree::onResourceSet() {
    motion.setBlendTree(std::dynamic_pointer_cast<BlendTree>(_resource));
}
void DocBlendTree::onPreSave() {
    auto res = std::dynamic_pointer_cast<BlendTree>(_resource);
    res->copy(&motion.getTree());
}

void DocBlendTree::guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
    bool clicked = false;
    bool selected = selected_node == node;
    std::string node_name = MKSTR(node->getDesc().name << "###" << node);
    gfxm::vec3 col = node->getDesc().color;
    ImGuiExt::BeginTreeNode(node_name.c_str(), pos, &clicked, selected, ImVec2(200, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1.0f)));

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
    if(clicked) {
        selected_node = node;
    }
}
std::map<JobGraphNode*, ImVec2> node_poses;
void DocBlendTree::onGui(Editor* ed, float dt) {
    auto& blendTree = motion.getTree();
    
    motion.advance(dt);
    auto& pose = motion.getPose();

    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(auto n : blendTree.getGraph().getNodes()) {
            guiDrawNode(blendTree.getGraph(), n, &node_poses[n]);
        }
    }
    ImGuiExt::EndGridView();    
    ImGui::NextColumn();

    std::vector<ktNode*> tgt_nodes;
    if(skel) {
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


    if(cam_pivot) {
        viewport.camSetPivot(cam_pivot->getTransform()->getWorldPosition());
    }
    if(cam_light) {
        cam_light->getOwner()->getTransform()->setTransform(gfxm::inverse(viewport.getView()));
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

                    cam_light = scn.createChild()->get<DirLight>().get();
                    cam_light->intensity = 20.0f;

                    viewport.resetCamera((box.from + box.to) * 0.5f, gfxm::length(box.to - box.from));
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndColumns();
}
void DocBlendTree::onGuiToolbox(Editor* ed) {
    auto& blendTree = motion.getTree();

    if(ImGui::Button(ICON_MDI_PLUS " Add node")) {
        ImGui::OpenPopup("test");
    }
    if(ImGui::BeginPopup("test")) {
        if(ImGui::MenuItem("Multiply")) {
            blendTree.getGraph().addNode(new MultiplyJob);
            blendTree.getGraph().prepare();
        }
        if(ImGui::MenuItem("Printer")) {
            blendTree.getGraph().addNode(new PrintJob);
            blendTree.getGraph().prepare();
        }
        if(ImGui::MenuItem("Test")) {
            blendTree.getGraph().addNode(new TestJob);
            blendTree.getGraph().prepare();
        }
        if(ImGui::MenuItem("Float")) {
            blendTree.getGraph().addNode(new FloatNode);
            blendTree.getGraph().prepare();
        }
        if(ImGui::MenuItem("SingleAnim")) {
            auto node = blendTree.createNode<SingleAnimJob>();
            node->setSkeleton(skel);
            blendTree.getGraph().prepare();
        }
        if(ImGui::MenuItem("Blend3")) {
            blendTree.getGraph().addNode(new Blend3Job);
            blendTree.getGraph().prepare();
        }
        ImGui::EndPopup();
    }


    if(selected_node) {
        ImGui::BeginGroup();
        selected_node->onGui();
        ImGui::EndGroup();
    }

    imguiResourceTreeCombo("reference", ref_scn, "so", [](){

    });
    imguiResourceTreeCombo("skeleton", skel, "skl", [this](){
        motion.setSkeleton(skel);
    });
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });
}