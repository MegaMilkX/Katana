#ifndef SKYBOX_GRADIENT_HPP
#define SKYBOX_GRADIENT_HPP

#include "gfxm.hpp"
#include "resource/cube_map.hpp"

class SkyboxGradient {
public:
    SkyboxGradient() {
        color = gfxm::vec3(0.039f, 0.039f, 0.039f);
        color2 = gfxm::vec3(0.192f, 0.192f, 0.192f);
        color3 = gfxm::vec3(0.776f, 0.905f, 1.0f);
        updCubeMap();
    }

    GLuint getSkyMap() {
        return cube_map.getId();
    }
    GLuint getIrradianceMap() {
        return cube_map.getId();
    }

    void _editorGui() {
        if(ImGui::ColorEdit3("SkyColor", (float*)&color)) {
            updCubeMap();
        }
        if(ImGui::ColorEdit3("SkyColor2", (float*)&color2)) {
            updCubeMap();
        }
        if(ImGui::ColorEdit3("SkyColor3", (float*)&color3)) {
            updCubeMap();
        }
    }
private:
    void updCubeMap() {
        unsigned char pix[9] = { 
            color.x * 255, color.y * 255, color.z * 255,
            color2.x * 255, color2.y * 255, color2.z * 255,
            color3.x * 255, color3.y * 255, color3.z * 255 
        };
        cube_map.data(pix, 1, 3, 3);
    }

    gfxm::vec3 color;
    gfxm::vec3 color2;
    gfxm::vec3 color3;
    CubeMap cube_map;
};

#endif
