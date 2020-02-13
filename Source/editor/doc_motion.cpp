#include "doc_motion.hpp"

#include "../common/util/imgui_helpers.hpp"


void DocMotion::resetGui() {
    AnimatorBase* animator = _resource->getAnimator();
    if(!animator) {
        gui_stack.clear();
        return;
    }

    auto type = animator->getType();
    if(type == ANIMATOR_FSM) {
        AnimFSM* fsm = (AnimFSM*)animator; // TODO:

        gui_stack.clear();
        pushGuiLayer(new MotionGuiFSM("", this, fsm));
    } else if(type == ANIMATOR_BLEND_TREE) {
        BlendTree* bt = (BlendTree*)animator; // TODO:

        gui_stack.clear();
        pushGuiLayer(new MotionGuiBlendTree("", this, bt));
    }    
}

void DocMotion::setReferenceObject(ktNode* node) {
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
        _resource->skeleton = skel_ref->skeleton;
        _resource->setSkeleton(skel_ref->skeleton);

        sample_buffer.resize(_resource->skeleton->boneCount());
    }
}


void DocMotion::pushGuiLayer(MotionGuiBase* gui) {
    gui_stack.push_back(std::unique_ptr<MotionGuiBase>(gui));
}


void DocMotion::onGui(Editor* ed, float dt) {    
    if(gui_stack.empty()) {
        ImGui::Text("Create new");
        if(ImGui::Button("Animation FSM")) {
            _resource->resetAnimator<AnimFSM>();
            resetGui();
        }
        ImGui::Text("OR");
        if(ImGui::Button("BlendTree")) {
            _resource->resetAnimator<BlendTree>();
            resetGui();
        }
    } else {
        if(gui_stack.size() > 1) {
            if(ImGui::SmallButton("root")) {
                gui_stack.resize(1);
            }
            for(size_t i = 1; i < gui_stack.size(); ++i) {
                ImGui::SameLine();
                ImGui::Text(">");
                ImGui::SameLine();
                if(ImGui::SmallButton( gui_stack[i]->getTitle().c_str() )) {
                    gui_stack.resize(i + 1);
                }
            }
        }
        gui_stack.back()->drawGui(ed, dt);
    }

    // === PREVIEW ===
    bool is_open = true;
    ImGui::Begin("Motion preview", &is_open, ImVec2(300, 200));

    std::vector<ktNode*> tgt_nodes;
    if(_resource->skeleton) {
        auto& skel = _resource->skeleton;

        tgt_nodes.resize(skel->boneCount());

        _resource->update(dt, sample_buffer);

        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        for(size_t i = 0; i < sample_buffer.size(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(sample_buffer[i].t);
                n->getTransform()->setRotation(sample_buffer[i].r);
                n->getTransform()->setScale(sample_buffer[i].s);
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
        //skel_ref->debugDraw(&viewport.getDebugDraw());
    }

    viewport.draw(&scn);

    ImGui::End();
}

void DocMotion::onGuiToolbox(Editor* ed) {
    imguiResourceTreeCombo("reference", _resource->reference, "so", [this](){
        if(_resource->reference) {
            setReferenceObject(_resource->reference.get());
        }
    });
    imguiResourceTreeCombo("skeleton", _resource->skeleton, "skl", [this](){
        _resource->setSkeleton(_resource->skeleton);

        sample_buffer.resize(_resource->skeleton->boneCount());
    });

    ImGui::Separator();

    ImGui::Text("Parameters");
    static bool param_edit_open = false;
    static bool debug_input_open = false;
    if(ImGui::Button(ICON_MDI_PENCIL " Edit", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
        param_edit_open = !param_edit_open;
    }
    if(ImGui::Button(ICON_MDI_GAMEPAD " Debug input", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
        debug_input_open = !debug_input_open;
    }


    ImGui::Separator();
    if(gui_stack.empty()) {

    } else {
        gui_stack.back()->drawToolbox(ed);
    }

    if(param_edit_open) {
        if(ImGui::Begin("Motion params", &param_edit_open, ImVec2(300, 400))) {
            if(ImGui::Button(ICON_MDI_PLUS, ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                
            }

        }
        ImGui::End();
    }

    if(debug_input_open) {
        ImGui::Begin("Motion debug input", &debug_input_open, ImVec2(300, 400));

        bool dummy = true;
        ImGui::Checkbox("Enable input", &dummy);
        ImGui::Text("velo: "); ImGui::SameLine(); ImGui::Button("Left stick");
        ImGui::Text("grounded toggle: "); ImGui::SameLine(); ImGui::Button("R1");
        ImGui::Text("on_hit trigger: "); ImGui::SameLine(); ImGui::Button("SQUARE");

        ImGui::End();
    }

    /*
    
    bool bDummy = true;
    ImGui::Checkbox("grounded", &bDummy);
    ImGui::Button("event 13", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f));
    ImGui::Button("event 536", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f));
    */
}