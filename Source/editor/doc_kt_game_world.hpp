#ifndef DOC_KT_GAME_WORLD_HPP
#define DOC_KT_GAME_WORLD_HPP


#include "editor_document.hpp"
#include "../common/game_world/game_world.hpp"

#include "../../common/renderer.hpp"
#include "../../common/gui_viewport.hpp"

#include "../common/game_world/util/assimp_to_actor.hpp"


class DocKtGameWorld : public EditorDocumentTyped<ktGameWorld> {
    std::shared_ptr<CubeMap> skybox_map;

    RendererPBR renderer;
    GuiViewport gvp;

    ktGameObject* selected = 0;
    ktActorNode* selected_node = 0;

public:
    DocKtGameWorld() {
        if(_resource) _resource->getDynamicsWorld()->getDebugDraw()->setDD(&gvp.getDebugDraw());
    }

    void onResourceSet() override {
        gvp.enableDebugDraw(true);

        _resource->getDynamicsWorld()->getDebugDraw()->setDD(&gvp.getDebugDraw());
    
    }

    void onGui(Editor* ed, float dt) override {
        _resource->update(&gvp.getDebugDraw());

        gvp.begin();
        DrawList dl;
        gfxm::frustum frustum;
        // TODO: Construct frustum
        _resource->getRenderScene()->updateDrawList(frustum, dl);
        renderer.draw(gvp.getViewport(), gvp.getProjection(), gvp.getView(), dl);
        
        gvp.end();
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".fbx")) {
                    auto actor = _resource->createObject<ktActor>();
                    actor->setName(node->getName().c_str());
                    assimpImportAsActor(actor, node->getFullName().c_str());
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    void onGuiToolbox(Editor* ed) override {
        imguiResourceTreeCombo("skybox", skybox_map, "hdr", [this](){
            renderer.setSkyCubemap(skybox_map);
        });

        if(ImGui::Button("Add ktActor")) {
            auto actor = _resource->createObject<ktActor>();
            actor->setName("ktActor");
        }
        if(ImGui::Button("Add ktCharacter")) {
            auto actor = _resource->createObject<ktActor>();
            actor->setName("ktCharacter");
        }

        ImGui::BeginChildFrame(ImGui::GetID("GameObjects"), ImVec2(0,150));
        for(auto o : _resource->game_objects) {
            if(ImGui::Selectable(o->getName().c_str(), selected == o)) {
                selected = o;
            }
        }
        ImGui::EndChildFrame();
        if(selected) {
            selected->onGui();
            ktActor* selected_actor = dynamic_cast<ktActor*>(selected);
            if(selected_actor) {
                ImGui::DragInt("very_cool", &selected_actor->very_cool.cool_value);
                ImGui::DragFloat("cool", &selected_actor->cool.my_value);

                if(ImGui::SmallButton("Add CoolComponent")) {
                    selected_actor->createComponent<ktCoolComponent>();
                }
                if(ImGui::SmallButton("Add LightOmniComponent")) {
                    selected_actor->createComponent<ktLightOmniComponent>();
                }
                if(ImGui::SmallButton("Add VeryCoolComponent")) {
                    selected_actor->createComponent<ktVeryCoolComponent>();
                }
                if(selected_actor->hasComponent<ktCoolComponent>()) {
                    auto p = selected_actor->getComponent<ktCoolComponent>();
                    if(ImGui::DragFloat("my_value", &p->my_value)) {

                    }
                }
                if(selected_actor->hasComponent<ktLightOmniComponent>()) {
                    auto p = selected_actor->getComponent<ktLightOmniComponent>();
                    if(ImGui::DragFloat3("color", (float*)&p->color)) {
                        
                    }
                }
                if(selected_actor->hasComponent<ktVeryCoolComponent>()) {
                    auto p = selected_actor->getComponent<ktVeryCoolComponent>();
                    if(ImGui::DragInt("cool", &p->cool_value)) {
                        
                    }
                }

                ImGui::BeginChildFrame(ImGui::GetID("ActorNodes"), ImVec2(0,150));
                std::function<void(ktActorNode*)> nodeTree;
                nodeTree = [this, &selected_actor, &nodeTree](ktActorNode* node){
                    if(!node->first_child) {
                        if(ImGui::Selectable(node->getName().c_str(), selected_node == node)) {
                            selected_node = node;
                        }
                        if(ImGui::BeginDragDropSource(0)) {
                            ImGui::SetDragDropPayload("DND_ACTOR_NODE", &node, sizeof(node));
                            ImGui::Text(node->getName().c_str());
                            ImGui::EndDragDropSource();
                        }
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ACTOR_NODE")) {
                                ecsEntityHandle hdlp = *(ecsEntityHandle*)payload->Data;
                                // TODO: Add dragged node as a child to this node
                            }
                            ImGui::EndDragDropTarget();
                        }
                    } else {
                        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
                        if(selected_node == node) {
                            tree_node_flags |= ImGuiTreeNodeFlags_Selected;
                        }
                        bool open = ImGui::TreeNodeEx(MKSTR(node->getName() << "###" << node).c_str(), tree_node_flags);
                        if(ImGui::BeginDragDropSource(0)) {
                            ImGui::SetDragDropPayload("DND_ACTOR_NODE", &node, sizeof(node));
                            ImGui::Text(node->getName().c_str());
                            ImGui::EndDragDropSource();
                        }
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ACTOR_NODE")) {
                                ecsEntityHandle hdlp = *(ecsEntityHandle*)payload->Data;
                                // TODO: Add dragged node as a child to this node
                            }
                            ImGui::EndDragDropTarget();
                        }
                        // TODO: Context menu here
                        if(ImGui::IsItemClicked(0)) {
                            selected_node = node;
                        }
                        if(open) {
                            std::vector<ktActorNode*> children;
                            ktActorNode* child = node->first_child;
                            while(child != 0) {
                                children.push_back(child);
                                child = child->next_sibling;
                            }
                            for(int i = 0; i < children.size(); ++i) {
                                nodeTree(children[i]);
                            }
                            ImGui::TreePop();
                        }
                    }
                };
                nodeTree(selected_actor->getRoot());
                ImGui::EndChildFrame();

                if(selected_node) {
                    if(ImGui::Button("Create ktNodeRigidBody")) {
                        auto rb = new ktNodeRigidBody(selected_actor);
                        rb->setName("ktNodeRigidBody");
                        selected_actor->addChild(selected_node, rb);
                    }
                    if(ImGui::Button("Create ktNodeMesh")) {
                        auto mesh = new ktNodeMesh(selected_actor);
                        mesh->setName("ktNodeMesh");
                        selected_actor->addChild(selected_node, mesh);
                    }
                    selected_node->onGui();
                }
            }
        }
    }
};
STATIC_RUN(DocKtGameWorld) {
    regEditorDocument<DocKtGameWorld>({ "world" });
}


#endif
