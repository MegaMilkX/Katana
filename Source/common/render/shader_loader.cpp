#include "shader_loader.hpp"

#include "../shader_factory.hpp"

#include "util/shader_preprocessor.hpp"

#include "../util/log.hpp"

#include "../platform/platform.hpp"

ShaderLoader& shaderLoader() {
    static ShaderLoader sl;
    return sl;
}


std::string loadShaderFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        LOG_WARN("File " << path << " not found");
        return std::string();
    }
    std::string buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return buf;
}

std::string ShaderLoader::getShaderSource (const char* path, bool force_reload) {
    std::string& source = std::string();
    auto it = source_cache.find(path);
    if(it != source_cache.end() && !force_reload) {
        source = it->second;
    } else {
        source = loadShaderFile((get_module_dir() + "/" + platformGetConfig().data_dir + "/" + path).c_str());
        if(source.empty()) {
            LOG_ERR("Shader source file not found: '" << path << "'");
            return std::string();
        }
        source_cache[path] = source;
    }
    return source;
}

gl::ShaderProgram* ShaderLoader::loadShaderProgram(const char* path, bool force_reload) {
    std::string source = getShaderSource(path, force_reload);
    if (source.empty()) {
        return 0;
    }
    auto it2 = shader_cache.find(path);
    if(it2 != shader_cache.end() && !force_reload) {
        return it2->second;
    } else {
        static bool f_reload;
        f_reload = force_reload;
        auto cb = [](const char* path)->std::string {
            return shaderLoader().getShaderSource(path, f_reload);
        };
        std::string vs, ps;
        preprocessShaderSource(source.c_str(), path, vs, ps, cb);
        gl::ShaderProgram* prog = ShaderFactory::getOrCreate(path, vs, ps, force_reload);
        if(prog) {
            shader_cache[path] = prog;
            return prog;
        }
    }
    
    return 0;
}

void ShaderLoader::reloadAll (void) {
    for(auto it = shader_cache.begin(); it != shader_cache.end(); ++it) {
        loadShaderProgram(it->first.c_str(), true);
    }
}