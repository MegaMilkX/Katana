#include "attrib_type_lib.hpp"

void AttribTypeLib::add(const std::string& category, rttr::type t) {
    table[category].emplace_back(t);
}
std::map<std::string, std::vector<rttr::type>>& AttribTypeLib::getTable(){
    return table;
}

AttribTypeLib& getAttribTypeLib() {
    static AttribTypeLib lib;
    return lib;
}