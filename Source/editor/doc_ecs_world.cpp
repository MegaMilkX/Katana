#include "doc_ecs_world.hpp"

#include "../common/util/filesystem.hpp"
#include "../common/ecs/storage/storage_transform.hpp"


DocEcsWorld::DocEcsWorld()
: mode(new DocEcsWorldMode3d()) 
{
    //foo_render();

    imgui_win_flags |= ImGuiWindowFlags_MenuBar;

    setIconCode(ICON_MDI_ALPHA_W_BOX);

    _resource->getSystem<ecsysAnimation>();
    _resource->getSystem<ecsysSceneGraph>();
    _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&state.gvp.getDebugDraw());
    renderSys = _resource->getSystem<ecsRenderSystem>();
    _resource->getSystem<ecsRenderGui>();

    auto ent = _resource->createEntity();
    ent.getAttrib<ecsVelocity>();

    state.gvp.camMode(GuiViewport::CAM_PAN);

    bindActionPress("ALT", [this](){ 
        state.gvp.camMode(GuiViewport::CAM_ORBIT); 
    });
    bindActionRelease("ALT", [this](){ state.gvp.camMode(GuiViewport::CAM_PAN); });
    bindActionPress("CTRL", [this](){ inp_mod_ctrl = true; });
    bindActionRelease("CTRL", [this](){ inp_mod_ctrl = false; });
    bindActionPress("Z", [this](){ if(inp_mod_ctrl){ state.restoreState(); } });
}

void DocEcsWorld::onResourceSet() {
    _resource->getSystem<ecsysAnimation>();
    _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&state.gvp.getDebugDraw());
    _resource->getSystem<ecsysSceneGraph>();
}

void DocEcsWorld::onGui(Editor* ed, float dt) {        
    if(subscene_stack.size() > 0) {
        if(ImGui::SmallButton("root")) {
            subscene_stack.clear();
            state.selected_ent = 0;
        }
        for(size_t i = 0; i < subscene_stack.size(); ++i) {
            ImGui::SameLine();
            ImGui::Text(">");
            ImGui::SameLine();
            if(ImGui::SmallButton( MKSTR(subscene_stack[i]).c_str() )) {
                subscene_stack.resize(i + 1);
                state.selected_ent = 0;
            }
        }
    }

    if(!subscene_stack.empty()) {
        state.world = subscene_stack.back()->world;
        state.undo_stack = &subscene_stack.back()->undo_stack;
    } else {
        state.world = _resource.get();
        state.undo_stack = &root_world_undo_stack;
    }
    
    _resource->update();

    if(ImGui::BeginMenuBar()) {
        ImGui::PushItemWidth(100);
        if(ImGui::BeginCombo("mode", mode->getName())) {
            if(ImGui::Selectable("3D")) {
                mode.reset(new DocEcsWorldMode3d());
            }
            ImGui::Selectable("2D");
            if(ImGui::Selectable("GUI")) {
                mode.reset(new DocEcsWorldModeGui());
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        mode->onMenuBar(state);
        ImGui::EndMenuBar();
    }

    auto renderSys = state.world->getSystem<ecsRenderSystem>();
    state.dl.clear();
    renderSys->fillDrawList(state.dl);

    mode->onMainWindow(state);
}

void DocEcsWorld::onGuiToolbox(Editor* ed) {
    if(!subscene_stack.empty()) {
        state.world = subscene_stack.back()->world;
        state.undo_stack = &subscene_stack.back()->undo_stack;
    } else {
        state.world = _resource.get();
        state.undo_stack = &root_world_undo_stack;
    }

    mode->onToolbox(state);
}

void DocEcsWorld::onFocus() {
    kt_cmd_set_callback("ecs_world_subdoc", std::bind(&DocEcsWorld::onCmdSubdoc, this, std::placeholders::_1, std::placeholders::_2));
}
void DocEcsWorld::onUnfocus() {
    kt_cmd_clear_callback("ecs_world_subdoc");
}

void DocEcsWorld::onCmdSubdoc(int argc, const char* argv[]) {
    if(argc < 2) {
        LOG_WARN("onCmdSubdoc: Not enough arguments");
        return;
    }

    // TODO: Handle exceptions
    uint64_t val = std::stoull(argv[1]);
    ecsWorld* ptr = (ecsWorld*)val;

    if(!ptr) {
        return;
    }

    std::shared_ptr<SubWorldContext> sptr(new SubWorldContext);
    sptr->world = ptr;
    
    subscene_stack.push_back(sptr);
    state.selected_ent = 0;
}