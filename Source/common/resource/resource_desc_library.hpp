#ifndef RESOURCE_DESC_LIBRARY_HPP
#define RESOURCE_DESC_LIBRARY_HPP

#include <map>
#include <string>

#include <rttr/type>

#include "resource_desc.hpp"

#include "../util/singleton.hpp"

#include "../resource/texture2d.h"

typedef std::shared_ptr<Texture2D>(*create_preview_fn_t)(const std::string& res);

class ResourceDescLibrary : public Singleton<ResourceDescLibrary> {
    std::map<rttr::type, create_preview_fn_t>      type_to_preview_creator;
    std::map<rttr::type, int>                      type_flags;
    std::map<std::string, rttr::type>              ext_to_type;

public:
    enum FLAGS {
      FLAG_NONE = 0,
      FLAG_VIEWABLE = 1,
      FLAG_WRITABLE = 2
    };

    void init();

    template<typename T>
    ResourceDescLibrary& add(const char* ext, int flags, create_preview_fn_t preview_fn = nullptr) {
        rttr::type t = rttr::type::get<T>();
        type_to_preview_creator[t] = preview_fn;
        type_flags[t] = flags;
        ext_to_type.insert(std::make_pair(std::string(ext), t));
        return *this;
    }
    template<typename T>
    ResourceDescLibrary& add(const std::vector<std::string>& extensions, int flags, create_preview_fn_t preview_fn = nullptr) {
        for(auto& ext : extensions) {
            add<T>(ext.c_str(), flags, preview_fn);
        }
        return *this;
    }

    std::string getExtension(const std::string& res_name);
    rttr::type findType(const std::string& res_path_or_ext);

    std::shared_ptr<Texture2D> createPreview(const std::string& res_path);

    int getFlags(rttr::type t);
};

#endif
