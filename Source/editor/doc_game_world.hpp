#ifndef DOC_GAME_WORLD_HPP
#define DOC_GAME_WORLD_HPP

#include "editor_document.hpp"
#include "../common/game_world/game_world.hpp"
#include "../common/game_world/static_model.hpp"
#include "../common/game_world/rigid_body.hpp"


class DocGameWorld : public EditorDocumentTyped<ktGameWorld> {
    GuiViewport gvp;
    ktActor* selected_actor = 0;
public:
    DocGameWorld() {
        if(_resource) {
            _resource->debugDrawer.setDD(&gvp.getDebugDraw());

            btCollisionObject* co = new btCollisionObject;
            btCollisionShape* shape = new btSphereShape(0.5f);
            co->setCollisionShape(shape);
            _resource->bt_world->addCollisionObject(co);
        }
    }
    void onResourceSet() override {
        _resource->debugDrawer.setDD(&gvp.getDebugDraw());
    }
    void onGui(Editor* ed, float dt) override {
        _resource->update(dt);
        
        DrawList dl;
        if(gvp.begin()) {
            gvp.getRenderer()->draw(gvp.getViewport(), gvp.getProjection(), gvp.getView(), dl);
            //fb_silhouette.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            //fb_outline.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            //fb_blur.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            //fb_pick.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
        }
        gvp.end();
    }
    void onGuiToolbox(Editor* ed) override {
        ktGameWorld* world = _resource.get();
        
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginCombo("###CREATE_ACTOR", ICON_MDI_PLUS " Create Actor")) {
            auto actor_type = rttr::type::get<ktActor>();
            for(auto& a_type : actor_type.get_derived_classes()) {
                if(ImGui::Selectable(a_type.get_name().to_string().c_str())) {
                    auto variant = a_type.create();
                    if(variant.can_convert<ktActor*>()) {
                        auto ptr = variant.get_value<ktActor*>();
                        selected_actor = ptr;
                        world->addActor(ptr);
                    } else {
                        LOG_ERR("Type " << variant.get_type().get_name().to_string() << " cannot be converted to ktActor*");
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        // Actor list
        ImGui::Text("Actor list");
        ImGui::PushItemWidth(-1);
        // TODO: Actor groups, purely for editor, no logical impact
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 0))) {
            bool open = ImGui::TreeNode(ICON_MDI_FOLDER " Unsorted");
            if(open) {
                auto actor_count = world->actorCount();
                for(int i = 0; i < actor_count; ++i) {
                    auto actor = world->getActor(i);
                    std::string name = (std::ostringstream() << actor->getName() << "###" << actor).str();
                    if(ImGui::Selectable(name.c_str(), selected_actor == actor)) {
                        selected_actor = actor;
                    }
                }
                ImGui::TreePop();
            }
            

            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();

        if(selected_actor) {
            selected_actor->onGui();
        }

        return;
    }

    void onFocus() override {}
    void onUnfocus() override {}
};


#endif
