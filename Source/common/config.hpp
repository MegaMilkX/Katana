#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <string>
#include "gfxm.hpp"
#include "util/filesystem.hpp"
#include "lib/inih/INIReader.h"
#include "util/log.hpp"

class Config {
    void load() {
        INIReader reader((get_module_dir() + "/config.ini").c_str());
        if(reader.ParseError() < 0) {
            LOG_WARN("Failed to load config: " << reader.ParseError());
            return;
        }
        data_dir = reader.Get("General", "data_dir", ".");
        // TODO:
    }
public:
    std::string data_dir = ".";
    gfxm::ivec2 resolution = gfxm::ivec2(640, 480);

    Config() {
        load();
    }
};

#endif
