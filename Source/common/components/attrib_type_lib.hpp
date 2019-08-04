#ifndef ATTRIB_TYPE_LIB_HPP
#define ATTRIB_TYPE_LIB_HPP

#include <rttr/type>
#include <string>
#include <map>
#include <vector>

extern std::map<std::string, std::vector<rttr::type>> g_attribTypeLibrary;

class AttribTypeLib {
    std::map<std::string, std::vector<rttr::type>> table;
public:
    void add(const std::string& category, rttr::type t);
    std::map<std::string, std::vector<rttr::type>>& getTable();
};

AttribTypeLib& getAttribTypeLib();

#endif
