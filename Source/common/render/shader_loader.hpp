#ifndef SHADER_LOADER_HPP
#define SHADER_LOADER_HPP

#include "../gl/shader_program.h"


class ShaderLoader {
    std::map<std::string, std::string>          source_cache;
    std::map<std::string, gl::ShaderProgram*>   shader_cache;
public:
    std::string        getShaderSource   (const char* path, bool force_reload = false);
    gl::ShaderProgram* loadShaderProgram (const char* path, bool force_reload = false);

    void               reloadAll         (void);
};

ShaderLoader& shaderLoader();


#endif

