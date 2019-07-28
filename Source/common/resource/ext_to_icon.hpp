#ifndef EXT_TO_ICON_HPP
#define EXT_TO_ICON_HPP

#include <map>
#include <string>

#include "../util/materialdesign_icons.hpp"

const std::map<std::string, const char*> g_ext_to_icon_code = {
    { ".png", ICON_MDI_FILE_IMAGE },
    { ".jpg", ICON_MDI_FILE_IMAGE },
    { ".jpeg", ICON_MDI_FILE_IMAGE },
    { ".jfif", ICON_MDI_FILE_IMAGE },
    { ".tga", ICON_MDI_FILE_IMAGE },
    
    { ".mat", ICON_MDI_FORMAT_COLOR_FILL },
    
    { ".msh", ICON_MDI_CUBE },
    
    { ".anm", ICON_MDI_VECTOR_CURVE },
    
    { ".so", ICON_MDI_EARTH },

    { ".fbx", ICON_MDI_PACKAGE_VARIANT },

    { ".ogg", ICON_MDI_MUSIC_NOTE }
};

inline const char* getExtIconCode(const char* ext) {
    if(!ext) return ICON_MDI_FILE;
    auto it = g_ext_to_icon_code.find(ext);
    if(it != g_ext_to_icon_code.end()) {
        return it->second;
    }
    return ICON_MDI_FILE;
}

#endif
