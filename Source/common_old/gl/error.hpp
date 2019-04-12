#ifndef GL_ERROR_HPP
#define GL_ERROR_HPP

#include <string>
#include "../util/log.hpp"
#include "glextutil.h"

namespace gl {

inline void checkError(const std::string& from) {
    auto err = glGetError();
    if(err) LOG(from << ": " << "GL error: " << err);
}

#define GL_LOG_ERROR(LABEL) gl::checkError(MKSTR(__FUNCTION__ << ", " << LABEL))

};

#endif
