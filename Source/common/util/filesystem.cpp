#include "filesystem.hpp"

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