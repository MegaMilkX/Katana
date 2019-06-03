#ifndef TEXTURE_VIEW_HPP
#define TEXTURE_VIEW_HPP

#include "asset_view.hpp"
#include "../common/resource/texture2d.h"

class TextureAssetView : public AssetView {
public:
    TextureAssetView(std::shared_ptr<Texture2D> tex)
    : tex(tex) {

    }

    virtual void onGui() {
        ImGui::Image(
            (ImTextureID)tex->GetGlName(), 
            ImVec2(200, 200), 
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }
private:
    std::shared_ptr<Texture2D> tex;
};

#endif
