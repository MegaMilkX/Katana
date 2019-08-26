#ifndef MAKE_UNIQUE_STRING_HPP
#define MAKE_UNIQUE_STRING_HPP

#include <string>
#include <set>

inline std::string makeUniqueString(const std::set<std::string>& names, const std::string& name) {
    std::string result = name;
    for(auto& a : names) {
        if(a == name) {
            std::string action_name = a;
            size_t postfix_pos = std::string::npos;
            for(int i = action_name.size() - 1; i >= 0; --i) {
                if((action_name[i] >= '0') && (action_name[i] <= '9')) {
                    postfix_pos = i;
                } else {
                    break;
                }
            }
            
            if(postfix_pos == std::string::npos) {
                action_name = action_name + "_1";
            } else {
                std::string postfix = action_name.substr(postfix_pos);
                action_name = action_name.substr(0, postfix_pos);
                int postfix_int = std::stoi(postfix);
                action_name = action_name + std::to_string(postfix_int + 1);
            }
            
            result = makeUniqueString(names, action_name);

            break;
        }
    }
    return result;
}

#endif
