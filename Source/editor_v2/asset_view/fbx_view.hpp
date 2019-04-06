#ifndef FBX_VIEW_HPP
#define FBX_VIEW_HPP

#include "asset_view.hpp"

class FbxView : public AssetView {
public:
    virtual void onGui() {
        ImGui::Text("FBX file");
    }
};

#endif
