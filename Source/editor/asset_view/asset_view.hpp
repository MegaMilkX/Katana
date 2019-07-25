#ifndef ASSET_VIEW_HPP
#define ASSET_VIEW_HPP

#include "../../common/lib/imgui_wrap.hpp"

#include "../../common/util/log.hpp"

#include "../../common/resource/resource_tree.hpp"

class AssetView {
public:
    virtual ~AssetView() {}
    virtual void onGui() {
        ImGui::Text("No editor for selected asset");
    }
    virtual void save() {

    }
    virtual void reload() {
        
    }
};

#endif
