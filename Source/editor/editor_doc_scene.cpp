#include "editor_doc_scene.hpp"

#include "../common/scene/game_scene.hpp"

#include "../common/scene/controllers/render_controller.hpp"
#include "../common/scene/controllers/dynamics_ctrl.hpp"
#include "../common/scene/controllers/constraint_ctrl.hpp"
#include "../common/scene/controllers/audio_controller.hpp"
#include "../common/scene/controllers/anim_controller.hpp"

#include "editor.hpp"

#include "../common/katana_impl.hpp"
extern std::unique_ptr<KatanaImpl> kt_play_mode;

EditorDocScene::EditorDocScene() {
    imgui_win_flags = ImGuiWindowFlags_MenuBar;

    gvp.enableDebugDraw(false);

    bindActionPress("ALT", [this](){ 
        gvp.camMode(GuiViewport::CAM_ORBIT); 
    });
    bindActionRelease("ALT", [this](){ gvp.camMode(GuiViewport::CAM_PAN); });

    bindActionPress("Z", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        
        if(selected.empty()) {
            gvp.resetCamera(
                gfxm::vec3(.0f, .0f, .0f),
                5.0f
            );
            return;
        }
        ktNode* o = *selected.getAll().begin();
        o->refreshAabb();

        gvp.resetCamera(
            gfxm::lerp(o->getAabb().from, o->getAabb().to, 0.5f),
            gfxm::length(o->getAabb().to - o->getAabb().from) * 0.8f + 0.01f
        );
    });
}


void EditorDocScene::onFocus() {
    if(_resource) {
        std::function<void(ktNode*)> update_instances_recursive_fn;
        update_instances_recursive_fn = [&update_instances_recursive_fn](ktNode* o) {
            if(o->getType() == OBJECT_INSTANCE) {
                auto inst = (ktObjectInstance*)o;
                inst->setScene(inst->getScene());
            } else {
                for(size_t i = 0; i < o->childCount(); ++i) {
                    update_instances_recursive_fn(o->getChild(i));
                }
            }
        };

        update_instances_recursive_fn(_resource.get());
    }
}

void EditorDocScene::onGui (Editor* ed, float dt) {
    auto& scene = _resource;
    scene->getController<DynamicsCtrl>();
    if(running) {
        scn_ptr = world.getScene();
        world.update(dt);
    } else {
        scn_ptr = scene.get();
    }

    if(ImGui::BeginMenuBar()) {
        bool dd = gvp.debugDrawEnabled();
        if(ImGui::Checkbox("Debug draw", &dd)) {
            gvp.enableDebugDraw(dd);
        }
        if(!running) {
            if(ImGui::Button(ICON_MDI_PLAY "Run")) {
                world.getScene()->copy(scene.get());
                world.start();
                running = true;
            }
        } else {
            if(ImGui::Button(ICON_MDI_STOP "Stop")) {
                world.cleanup();
                world.getScene()->clear();
                running = false;
                scn_ptr = scene.get();
                selected.clear();
            }
        }
        /*
        ImGui::PushItemWidth(200);
        std::string selected_game_mode_name = "<null>";
        if(selected_game_mode.is_valid()) {
            selected_game_mode_name = selected_game_mode.get_name().to_string();
        }
        
        if(ImGui::BeginCombo("", selected_game_mode_name.c_str())) {
            auto derived = rttr::type::get<ktGameMode>().get_derived_classes();
            for(auto d : derived) {
                if(ImGui::Selectable(rttr::type::get<ktGameMode>().get_name().to_string().c_str(), rttr::type::get<ktGameMode>() == selected_game_mode)) {
                    selected_game_mode = rttr::type::get<ktGameMode>();
                }
                if(ImGui::Selectable(d.get_name().to_string().c_str())) {
                    selected_game_mode = d;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if(ImGui::Button(ICON_MDI_PLAY "Run")) {
            rttr::variant v = selected_game_mode.create();
            if(v.is_valid()) {
                if(v.get_type().is_pointer()) {
                    auto mode = v.get_value<ktGameMode*>();
                    
                    dstream strm;
                    _resource->write(strm);
                    strm.jump(0);
                    mode->getScene().read(strm);
                    //mode->getScene().copy(_resource.get());

                    kt_play_mode.reset(new KatanaImpl());
                    kt_play_mode->run(mode);
                } else {
                    LOG_WARN("Invalid constructor policy for game mode " << selected_game_mode.get_name().to_string() << ", check reflection registration");
                }
            } else {
                LOG_WARN("Failed to create game mode " << selected_game_mode.get_name().to_string());
            }
        }
        */
        ImGui::EndMenuBar();
    }

    if(ImGui::IsRootWindowOrAnyChildFocused()) {
        // TODO: Shouldn't be here
        //ed->getResourceTree().setSelected(getNode());
        ed->setFocusedDocument(this);
    }

    scene->getController<DynamicsCtrl>()->updateBodyTransforms();
    gvp.draw(scn_ptr, 0, gfxm::ivec2(0,0));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_DND_RESOURCE)) {
            ResourceNode* node = *(ResourceNode**)payload->Data;
            LOG("Payload received: " << node->getFullName());
            if(has_suffix(node->getName(), ".so")) {
                auto o = scn_ptr->createInstance(node->getResource<GameScene>());
                o->getTransform()->setScale(
                    node->getResource<GameScene>()->getTransform()->getScale()
                );

                gfxm::vec3 pos = gvp.getMouseScreenToWorldPos(0);
                pos = gfxm::inverse(scn_ptr->getTransform()->getWorldTransform()) * gfxm::vec4(pos, 1.0f);
                gfxm::vec3 scale_new = o->getTransform()->getScale();
                gfxm::vec3 scale_world = scn_ptr->getTransform()->getScale();
                o->getTransform()->setScale(gfxm::vec3(scale_new.x / scale_world.x, scale_new.y / scale_world.y, scale_new.z / scale_world.z));
                o->getTransform()->translate(pos);

                selected.clearAndAdd(o);
            }
        }
        ImGui::EndDragDropTarget();
    }

    for(auto& o : selected.getAll()) {
        o->onGizmo(gvp);
    }
}

void EditorDocScene::onGuiToolbox(Editor* ed) {
    auto& scene = _resource;

    ImGuiID dock_id = ImGui::GetID(getName().c_str());
    ImGui::DockSpace(dock_id);

    const std::string win_scene_insp_name = MKSTR("Scene Inspector##" << getName());
    const std::string win_obj_insp_name = MKSTR("Object Inspector##" << getName());

    if(first_use) {
        ImGuiID dsid_up = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Up, 0.35f, NULL, &dock_id);

        ImGui::DockBuilderDockWindow(win_obj_insp_name.c_str(), dock_id);
        ImGui::DockBuilderDockWindow(win_scene_insp_name.c_str(), dsid_up);

        first_use = false;
    }

    scene_inspector.update(scn_ptr, selected, win_scene_insp_name);
    object_inspector.update(scn_ptr, selected, win_obj_insp_name);
}