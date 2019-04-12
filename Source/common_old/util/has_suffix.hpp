#ifndef HAS_SUFFIX_H
#define HAS_SUFFIX_H

#include <string>

inline bool has_suffix(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

#endif
