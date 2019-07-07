#ifndef INIT_FILESYSTEM_RESOURCES_HPP
#define INIT_FILESYSTEM_RESOURCES_HPP

#include <algorithm>
#include "filesystem.hpp"
#include "../resource/data_registry.h"
#include "log.hpp"
#include "../platform/platform.hpp"

inline void initFilesystemResources(const std::string& rootDir) {
    GlobalDataRegistry().Clear();

    std::vector<std::string> files =
        find_all_files(rootDir, "*.*");
        //find_all_files(rootDir, "*.scn;*.geo;*.anim;*.mat;*.png;*.jpg;*.jpeg");
    std::vector<std::string> resNames = files;
    for(auto& f : resNames) {
        f.erase(f.find_first_of(rootDir), rootDir.size());
        if(f[0] == '\\') f.erase(0, 1);
        std::replace(f.begin(), f.end(), '\\', '/');
    }

    for(size_t i = 0; i < files.size(); ++i) {
        for(size_t j = 0; j < resNames[i].size(); ++j) {
            resNames[i][j] = (std::tolower(resNames[i][j]));
        }
        GlobalDataRegistry().Add(
            resNames[i],
            DataSourceRef(new DataSourceFilesystem(files[i]))
        );
    }

    LOG("Found " << GlobalDataRegistry().Count() << " file data sources");
}

inline void initFilesystemResources() {
    initFilesystemResources(get_module_dir() + "/" + platformGetConfig().data_dir);
}

#endif
