#ifndef ATTRIB_TILED_ROOM_HPP
#define ATTRIB_TILED_ROOM_HPP

#include "component.hpp"
#include "../scene/node.hpp"
#include "model.hpp"

#include <limits>

class attribTiledRoomCtrl : public Attribute {
    RTTR_ENABLE(Attribute)

    std::map<std::pair<int, int>, ktNode*> tiles;

    // editor
    std::string current_tile;
    std::vector<std::string> preset_tiles;
public:
    attribTiledRoomCtrl() {
        preset_tiles = {
            "assets/tile.so",
            "assets/tile_wall_x.so",
            "assets/tile_wall_mx.so",
            "assets/tile_wall_z.so",
            "assets/tile_wall_mz.so",
            "assets/tile_wall_corner_xz.so",
            "assets/tile_wall_corner_mxmz.so",
            "assets/tile_wall_corner_mxz.so",
            "assets/tile_wall_corner_xmz.so"
        };
    }

    virtual void onGui() {
        if(ImGui::BeginCombo("current tile", current_tile.c_str())) {
            for(size_t i = 0; i < preset_tiles.size(); ++i) {
                if(ImGui::Selectable(preset_tiles[i].c_str())) {
                    current_tile = preset_tiles[i];
                }
            }            
            ImGui::EndCombo();
        }
    }

    virtual void onGizmo(GuiViewport& vp) {
        auto& dd = vp.getDebugDraw();
        gfxm::vec3 col(1,1,1);
        if(vp.isMouseClicked(0)) {
            col = gfxm::vec3(1,0,0);
        }
        auto mpos = vp.getMousePos();

        auto wpos = vp.getMouseScreenToWorldPos(0);
        wpos.x = std::round(wpos.x / 2.0f) * 2.0f;
        wpos.z = std::round(wpos.z / 2.0f) * 2.0f;

        gfxm::aabb box;
        box = gfxm::aabb(
            wpos - gfxm::vec3(1.0f, .0f, 1.0f),
            wpos + gfxm::vec3(1.0f, 2.0f, 1.0f)
        );

        dd.aabb(
            box,
            col
        );

        if(vp.isMouseClicked(0)) {
            ktNode*& o = tiles[std::make_pair(wpos.x / 2, wpos.z / 2)];
            if(!o) {
                o = getOwner()->createChild();
                o->read(current_tile);
                o->getTransform()->setPosition(wpos);
            }
        }
    }
};

#endif
