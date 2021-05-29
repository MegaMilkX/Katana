#include "doc_motion.hpp"

#include "../common/util/imgui_helpers.hpp"
#include "../common/util/imgui_ext.hpp"

#include "../common/ecs/systems/render.hpp"
#include "../common/ecs/systems/animation_sys.hpp"
#include "../common/ecs/systems/scene_graph.hpp"


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

void DocMotion::setReferenceObject(std::shared_ptr<EntityTemplate> tpl) {
    world.clearEntities();
                    
    ref_entity = world.createEntityFromTemplate(tpl->Name().c_str());
    //gfxm::aabb box;
    //scn.makeAabb(box);

    //cam_light = scn.createChild()->get<DirLight>().get();
    //cam_light->intensity = 500.0f;

    //viewport.resetCamera((box.from + box.to) * 0.5f, gfxm::length(box.to - box.from));

    auto animator =  ref_entity.getAttrib<ecsSubSceneAnimator>();
    animator->setMotion(_resource, true);

    //cam_pivot = 0;
}


DocMotion::DocMotion() {
    world.getSystem<ecsRenderSystem>();
    world.getSystem<ecsysAnimation>();
    world.getSystem<ecsysSceneGraph>();
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
/*
    std::vector<ktNode*> tgt_nodes;
    if(_resource->skeleton) {
        auto& skel = _resource->skeleton;

        tgt_nodes.resize(skel->boneCount());

        _resource->update(dt, sample_buf);

        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        for(size_t i = 0; i < sample_buf.sampleCount(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(sample_buf[i].t);
                n->getTransform()->setRotation(sample_buf[i].r);
                n->getTransform()->setScale(sample_buf[i].s);
            }
        }

        if(root_motion_node) {
            ktNode* root = scn.getRoot();


        }
    }*/
/*
    if(cam_pivot) {
        viewport.camSetPivot(cam_pivot->getTransform()->getWorldPosition());
    }
    if(cam_light) {
        cam_light->getOwner()->getTransform()->setTransform(gfxm::inverse(viewport.getView()));
    }*//*
    auto skel_ref = scn.find<SkeletonRef>();
    if(skel_ref) {
        //skel_ref->debugDraw(&viewport.getDebugDraw());
    }*/

    world.update(dt);
    DrawList dl;
    world.getSystem<ecsRenderSystem>()->fillDrawList(dl);
    if(viewport.begin()) {
        viewport.getRenderer()->draw(
            viewport.getViewport(), 
            viewport.getProjection(), 
            viewport.getView(), 
            dl
        );
        
        viewport.getDebugDraw().line(
            gfxm::vec3(0,0,0), 
            gfxm::vec3(0, 0, 0) + ref_entity.getAttrib<ecsSubSceneAnimator>()->sample_buffer.getRootMotionDelta().t * 20.0f, 
            gfxm::vec3(1,0,0), 
            DebugDraw::DEPTH_DISABLE
        );
    }
    viewport.end();

    ImGui::End();
}

void DocMotion::onGuiToolbox(Editor* ed) {
    imguiResourceTreeCombo("reference", reference, "entity", [this](){
        if(reference) {
            _resource->reference_path = reference->Name();
            setReferenceObject(reference);
        }
    });/*
    imguiObjectCombo("camera pivot", cam_pivot, &scn, [](){

    });*/

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
            int i = 0;
            for(auto& param : _resource->getBlackboard()) {
                ImGuiExt::InputText(MKSTR("name###" << "name" << i).c_str(), param.name);
                ImGui::DragFloat(MKSTR("value###" << "value" << i).c_str(), &param.value, 0.001f);
                ++i;
            }
            
            if(ImGui::Button(ICON_MDI_PLUS, ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                int hdl = _resource->getBlackboard().allocValue();
                _resource->getBlackboard().setName(hdl, "param");
            }

        }
        ImGui::End();
    }

    if(debug_input_open) {
        if(ImGui::Begin("Motion debug input", &debug_input_open, ImVec2(300, 400))) {
            bool dummy = true;
            ImGui::Checkbox("Enable input", &dummy);
            ImGui::Text("velo: "); ImGui::SameLine(); ImGui::Button("Left stick");
            ImGui::Text("grounded toggle: "); ImGui::SameLine(); ImGui::Button("R1");
            ImGui::Text("on_hit trigger: "); ImGui::SameLine(); ImGui::Button("SQUARE");
        }
        ImGui::End();
    }

    /*
    
    bool bDummy = true;
    ImGui::Checkbox("grounded", &bDummy);
    ImGui::Button("event 13", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f));
    ImGui::Button("event 536", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f));
    */
}

void DocMotion::onResourceSet() {
    if(!_resource->reference_path.empty()) {
        reference = getResource<EntityTemplate>(_resource->reference_path);
        setReferenceObject(reference);
    }
    resetGui();
}