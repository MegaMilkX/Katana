#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

#include "editor_resource_tree.hpp"

#include "../common/util/log.hpp"

#include "../common/attributes/skeleton_ref.hpp"

DocBlendTree::DocBlendTree() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(true);
}

void DocBlendTree::onResourceSet() {
    motion.setBlendTree(std::dynamic_pointer_cast<BlendTree>(_resource));

    if(motion.getTree().ref_object) {
        setReferenceObject(motion.getTree().ref_object.get());
    }
}
void DocBlendTree::onPreSave() {
    auto res = std::dynamic_pointer_cast<BlendTree>(_resource);
    res->copy(&motion.getTree());
}


void DocBlendTree::setReferenceObject(ktNode* node) {
    scn.clear();
                    
    scn.copy(node);
    scn.getTransform()->setScale(
        node->getTransform()->getScale()
    );
    gfxm::aabb box;
    scn.makeAabb(box);

    cam_light = scn.createChild()->get<DirLight>().get();
    cam_light->intensity = 500.0f;

    viewport.resetCamera((box.from + box.to) * 0.5f, gfxm::length(box.to - box.from));

    auto skel_ref = scn.find<SkeletonRef>();
    if(skel_ref && skel_ref->skeleton) {
        motion.getTree().ref_skel = skel_ref->skeleton;
        motion.setSkeleton(skel_ref->skeleton);
    }
}


void DocBlendTree::guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
    bool clicked = false;
    bool selected = selected_node == node;
    std::string node_name = MKSTR(node->getDesc().name << "###" << node);
    gfxm::vec3 col = node->getDesc().color;
    ImGuiExt::BeginTreeNode(node_name.c_str(), pos, &clicked, selected, ImVec2(200, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1.0f)));

    int64_t new_conn_tgt = 0;
    for(size_t j = 0; j < node->getDesc().ins.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeIn(node->getDesc().ins[j].name.c_str(), (int64_t)node->getInput(j), (int64_t)node->getInput(j)->source)) {
            node->connect(j, (JobOutput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }
    for(size_t j = 0; j < node->getDesc().outs.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeOut(node->getDesc().outs[j].name.c_str(), (int64_t)node->getOutput(j), 0)) {
            node->connect(j, (JobInput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }

    ImGuiExt::EndTreeNode();
    if(clicked) {
        selected_node = node;
    }
}

void DocBlendTree::onGui(Editor* ed, float dt) {
    auto& blendTree = motion.getTree();
    
    motion.advance(dt);
    auto& pose = motion.getPose();

    ImGui::BeginColumns("First", 2);
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(auto n : blendTree.getNodes()) {
            const gfxm::vec2& p = n->getPos();
            ImVec2 pos(p.x, p.y);
            guiDrawNode(blendTree, n, &pos);
            n->setPos(gfxm::vec2(pos.x, pos.y));
        }
    }
    ImGuiExt::EndGridView();    
    ImGui::NextColumn();

    std::vector<ktNode*> tgt_nodes;
    if(motion.getTree().ref_skel) {
        auto& skel = motion.getTree().ref_skel;

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
    auto skel_ref = scn.find<SkeletonRef>();
    if(skel_ref) {
        skel_ref->debugDraw(&viewport.getDebugDraw());
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
                    motion.getTree().ref_object = ref;

                    setReferenceObject(ref.get());
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
            blendTree.addNode(new MultiplyJob);
            blendTree.prepare();
        }
        if(ImGui::MenuItem("Printer")) {
            blendTree.addNode(new PrintJob);
            blendTree.prepare();
        }
        if(ImGui::MenuItem("Test")) {
            blendTree.addNode(new TestJob);
            blendTree.prepare();
        }
        if(ImGui::MenuItem("Float")) {
            blendTree.addNode(new FloatNode);
            blendTree.prepare();
        }
        if(ImGui::MenuItem("SingleAnim")) {
            blendTree.createNode<SingleAnimJob>();
            blendTree.prepare();
        }
        if(ImGui::MenuItem("Blend3")) {
            blendTree.addNode(new Blend3Job);
            blendTree.prepare();
        }
        ImGui::EndPopup();
    }


    if(selected_node) {
        ImGui::BeginGroup();
        selected_node->onGui();
        ImGui::EndGroup();
    }

    imguiResourceTreeCombo("reference", motion.getTree().ref_object, "so", [this](){
        if(motion.getTree().ref_object) {
            setReferenceObject(motion.getTree().ref_object.get());
        }
    });
    imguiResourceTreeCombo("skeleton", motion.getTree().ref_skel, "skl", [this](){
        motion.setSkeleton(motion.getTree().ref_skel);
    });
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });

    ImGui::Separator();
    ImGui::Text("Values");

    for(int i = 0; i < blendTree.valueCount(); ++i) {
        const char* name = blendTree.getValueName(i);
        float val = blendTree.getValue(i);
        if(ImGui::DragFloat(name, &val, .001f)) {
            blendTree.setValue(i, val);
        }
        // TODO: List values, allow changing and renaming
    }
}