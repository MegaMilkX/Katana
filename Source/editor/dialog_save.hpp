#ifndef DIALOG_SAVE_HPP
#define DIALOG_SAVE_HPP

#include <string>
#include "../common/lib/nativefiledialog/nfd.h"
#include "../common/util/has_suffix.hpp"

inline std::string dialogSave(const std::string& ext) {
    char* outPath;
    auto r = NFD_SaveDialog(ext.c_str(), NULL, &outPath);
    if(r == NFD_OKAY) {
        std::string filePath(outPath);
        if(!has_suffix(filePath, ("." + ext).c_str())) {
            filePath = filePath + ("." + ext).c_str();
        }

        return filePath;
    }
    return "";
}

#endif
