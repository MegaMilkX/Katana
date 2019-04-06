#include "editor_asset_inspector.hpp"

#include "../common/util/has_suffix.hpp"

#include "editor.hpp"

#include "asset_view/anm_view.hpp"
#include "asset_view/material_view.hpp"
#include "asset_view/fbx_view.hpp"

EditorAssetInspector::EditorAssetInspector() {
    asset_view.reset(new AssetView());
}

void EditorAssetInspector::setFile(const std::string& fname) {
    file_name = fname;
    if(has_suffix(fname, ".anm")) {
        auto a = retrieve<Animation>(fname);
        loaded_resource = a;
        asset_view.reset(new AnimAssetView(a));
    } else if(has_suffix(fname, ".mat")) {
        auto a = retrieve<Material>(fname);
        loaded_resource = a;
        asset_view.reset(new MaterialView(a));
    } else if(has_suffix(fname, ".fbx")) {
        asset_view.reset(new FbxView());
    } else {
        loaded_resource = std::shared_ptr<Resource>();
        asset_view.reset(new AssetView());
    }
}

void EditorAssetInspector::update(Editor* editor) {
    if(ImGui::Begin("Resource inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::MenuItem("Save")) {
                //asset_view->save();
                if(loaded_resource) {
                    loaded_resource->write_to_file(
                        get_module_dir() + "\\" + loaded_resource->Name()
                    );
                }
            }
            if(ImGui::MenuItem("Reload")) {
                //asset_view->reload();
                if(loaded_resource) {
                    GlobalResourceFactory().forceReload(loaded_resource->Name());
                }
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text(file_name.c_str());
        asset_view->onGui();
        
        ImGui::End();
    }
}