#include "filesystem.hpp"

#include <assert.h>


fs_path::fs_path() {

}
fs_path::fs_path(const char* path)
: fs_path(std::string(path)) {

}
fs_path::fs_path(const std::string& path) {
    std::vector<std::string> tokens;
    std::string token;
    for(int i = 0; i < path.size(); ++i) {
        auto c = path[i];
        auto c1 = path[i + 1];
        if (c == '\\' || c == '/') {
            
        } else {
            while(c != '\0' && c != '\\' && c != '/') {
                token.push_back(c);
                ++i;
                c = path[i];
            }
            if(!token.empty()) {
                if(token == ".." && !tokens.empty() && tokens.back() != "..") {
                    tokens.pop_back();
                } else {
                    tokens.push_back(token);
                }
                token.clear();
            }
        }
    }

    if(tokens.empty()) {
        return;
    }

    stack = tokens;

    str += tokens[0];
    for(int i = 1; i < tokens.size(); ++i) {
        str += "/" + tokens[i];
    }
}

fs_path fs_path::relative(const fs_path& other) {
    assert(!other.stack.empty());
    assert(!stack.empty());
    fs_path res;

    int separation_index = -1;
    for(int i = 0; i < stack.size() && i < other.stack.size(); ++i) {
        if(stack[i] != other.stack[i]) {
            break;
        }
        separation_index = i;
    }

    assert(separation_index != -1);
    
    std::string appendage;
    if(separation_index < stack.size() - 1) {
        int count = stack.size() - separation_index - 1;
        for(int i = 0; i < count; ++i) {
            appendage += std::string("../");
        }
    }

    std::string str;
    str += appendage;
    for(int i = separation_index + 1; i < other.stack.size(); ++i) {
        str += other.stack[i] + ((i == other.stack.size() - 1) ? "" : "/");
    }

    res = fs_path(str);

    return res;
}

const std::string& fs_path::string() const {
    return str;
}
const char*        fs_path::c_str() const {
    return str.c_str();
}


void createDirRecursive(const std::string& p) {
    auto sanitizeString = [](const std::string& str)->std::string {
        std::string name = str;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
            if(name[i] == '\\') {
                name[i] = '/';
            }
        }
        return name;
    };
    std::string path = sanitizeString(p); 

    size_t offset = 0;
    offset = path.find_first_of("/", offset);
    while(offset != path.npos) {
        std::string part(path.begin(), path.begin() + offset);
        CreateDirectoryA(part.c_str(), 0);
        offset = path.find_first_of("/", offset + 1); 
    }
    CreateDirectoryA(path.c_str(), 0);
}