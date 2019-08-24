#ifndef RESOURCE_DESC_LIBRARY_HPP
#define RESOURCE_DESC_LIBRARY_HPP

#include <map>
#include <string>

#include <rttr/type>

#include "resource_desc.hpp"

#include "../util/singleton.hpp"

class ResourceDescLibrary : public Singleton<ResourceDescLibrary> {    
    std::map<rttr::type, create_resource_doc_fn_t> type_to_doc_creator;
    std::map<rttr::type, int>                    type_flags;
    std::map<std::string, rttr::type>              ext_to_type;

public:
    enum FLAGS {
      FLAG_NONE = 0,
      FLAG_VIEWABLE = 1,
      FLAG_WRITABLE = 2
    };

    void init();

    template<typename T>
    ResourceDescLibrary& add(const char* ext, int flags, create_resource_doc_fn_t fn) {
        rttr::type t = rttr::type::get<T>();
        type_to_doc_creator[t] = fn;
        type_flags[t] = flags;
        ext_to_type.insert(std::make_pair(std::string(ext), t));
        return *this;
    }
    template<typename T>
    ResourceDescLibrary& add(const std::vector<std::string>& extensions, int flags, create_resource_doc_fn_t fn) {
        for(auto& ext : extensions) {
            add<T>(ext.c_str(), flags, fn);
        }
        return *this;
    }

    rttr::type findType(const std::string& ext);

    int getFlags(rttr::type t);

    EditorDocument* createEditorDocument(const std::string& res_path);
    EditorDocument* createEditorDocument(rttr::type t, std::shared_ptr<ResourceNode>& rnode);
};

#endif
