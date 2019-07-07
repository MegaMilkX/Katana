#ifndef FILESYSTEM_PATH_HPP
#define FILESYSTEM_PATH_HPP

#include <string>

class path {
    std::string _path;

    void sanitize(std::string& p) {

    }
public:
    path(const std::string& p)
    : _path(p) {
        sanitize(_path);
    }

    bool exists();
    bool isdir();
    bool isfile();
    path parent();
};

#endif
