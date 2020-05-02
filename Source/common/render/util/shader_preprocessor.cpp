#include "shader_preprocessor.hpp"

#include "../../lib/glsl-parser/parser.h"
#include <filesystem>
#include <set>
#include <assert.h>

#include "../../util/log.hpp"


std::string preprocessShaderIncludes(const char* str, const char* path, const char* rel_dir, std::set<std::string>& include_chain, shader_include_callback_t cb) {
    std::string result;

    std::experimental::filesystem::path full_dir_path(rel_dir);
    full_dir_path.append(path);
    std::vector<std::string> sanitized_full_path_stack;
    for (auto& a : full_dir_path) {
        if (a.compare("..") == 0) {
            if (!sanitized_full_path_stack.empty()) {
                sanitized_full_path_stack.pop_back();
            }
        }
        else {
            sanitized_full_path_stack.push_back(a.string());
        }
    }
    std::experimental::filesystem::path sanitized_full_path;
    for (auto& a : sanitized_full_path_stack) {
        sanitized_full_path.append(a);
    }

    if (include_chain.count(sanitized_full_path.string())) {
        assert(false, "Circular dependency");
        return "";
    }
    include_chain.insert(sanitized_full_path.string());

    std::experimental::filesystem::path sanitized_full_dir_path(sanitized_full_path);
    sanitized_full_dir_path.remove_filename();
  
    glsl::lexer lex(str);
    glsl::token tok = lex.read();
    while (tok.m_type != glsl::kType_eof) {
        if (tok.m_type == glsl::kType_preprocessor) {
            if (tok.asPreprocessorDirective == std::string("#include")) {
                tok = lex.read();
                while (tok.m_type == glsl::kType_whitespace) {
                    tok = lex.read();
                }
                if (tok.m_type == glsl::kType_string_literal) {
                    assert(tok.length > 2, "Empty string not allowed");
                    std::string include_path(&str[tok.pos + 1], &str[tok.pos + tok.length - 1]);
                    std::experimental::filesystem::path inc_path(sanitized_full_dir_path);
                    inc_path.append(include_path);
                    std::string source = cb(inc_path.string().c_str());
                    if (!source.empty()) {
                        source = preprocessShaderIncludes(source.c_str(), include_path.c_str(), sanitized_full_dir_path.string().c_str(), include_chain, cb);
                        result.insert(result.end(), source.begin(), source.end());
                    }
                } else if (tok.m_type == glsl::kType_operator && tok.asOperator == glsl::kOperator_less) {
                    std::string include_path;
                    tok = lex.read();
                    while (!(tok.m_type == glsl::kType_operator && tok.asOperator == glsl::kOperator_greater)) {
                        assert(tok.m_type != glsl::kType_eof, "Unexpected end of file");
                        include_path.insert(include_path.end(), &str[tok.pos], &str[tok.pos + tok.length]);
                        tok = lex.read();
                    }
                    std::experimental::filesystem::path inc_path(sanitized_full_dir_path);
                    inc_path.append(include_path);
                    std::string source = cb(inc_path.string().c_str());
                    if (!source.empty()) {
                        source = preprocessShaderIncludes(source.c_str(), include_path.c_str(), sanitized_full_dir_path.string().c_str(), include_chain, cb);
                        result.insert(result.end(), source.begin(), source.end());
                    }
                } else {
                    assert(false, "Missing #include path");
                }
            } else {
                result.insert(result.end(), &str[tok.pos], &str[tok.pos + tok.length]);
            }
        } else {
            result.insert(result.end(), &str[tok.pos], &str[tok.pos + tok.length]);
        }

        tok = lex.read();
    }

    include_chain.erase(sanitized_full_path.string());
    return result;
}

void preprocessShaderSource(const char* src, const char* path, std::string& vs, std::string& fs, shader_include_callback_t cb) {
    glsl::lexer lex(src);
    int tok_type = glsl::kType_eof;
    glsl::token tok = lex.read();
    while(tok.m_type != glsl::kType_eof) {
        if (tok.m_type == glsl::kType_preprocessor) {
            LOG_WARN(std::string(&src[tok.pos], &src[tok.pos + tok.length]));
        } else if (tok.m_type == glsl::kType_string_literal) {
            LOG_WARN(std::string(&src[tok.pos], &src[tok.pos + tok.length]));
        }
        tok_type = tok.m_type;

        if (tok.m_type == glsl::kType_preprocessor) {
            if (tok.asPreprocessorDirective == std::string("#vertex")) {
                tok = lex.read();
                bool cont = false;
                while (tok.m_type != glsl::kType_eof) {
                    if (tok.m_type == glsl::kType_preprocessor && tok.asPreprocessorDirective == std::string("#fragment")) {
                        cont = true;
                        break;
                    }
                    vs.insert(vs.end(), &src[tok.pos], &src[tok.pos + tok.length]);
                    tok = lex.read();
                }
                if (cont) {
                    continue;
                }
            } else if (tok.asPreprocessorDirective == std::string("#fragment")) {
                tok = lex.read();
                bool cont = false;
                while (tok.m_type != glsl::kType_eof) {
                    if (tok.m_type == glsl::kType_preprocessor && tok.asPreprocessorDirective == std::string("#vertex")) {
                        cont = true;
                        break;
                    }
                    fs.insert(fs.end(), &src[tok.pos], &src[tok.pos + tok.length]);
                    tok = lex.read();
                }
                if (cont) {
                    continue;
                }
            }
        }

        glsl::token tok = lex.read();
    }

    if(vs.empty() && fs.empty()) {
        assert(false);
        return;
    }

    // Handle includes
    std::set<std::string> include_chain;
    if(!vs.empty()) {
        vs = preprocessShaderIncludes(vs.c_str(), path, "", include_chain, cb);
    }
    include_chain.clear();
    if(!fs.empty()) {
        fs = preprocessShaderIncludes(fs.c_str(), path, "", include_chain, cb);
    }
}
