#ifndef SHADER_PREPROCESSOR_HPP
#define SHADER_PREPROCESSOR_HPP

#include <string>

typedef std::string(*shader_include_callback_t)(const char* path);

void preprocessShaderSource(const char* src, const char* path, std::string& vs, std::string& ps, shader_include_callback_t cb);


#endif
