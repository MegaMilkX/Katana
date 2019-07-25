#ifndef RESOURCE_DESC_HPP
#define RESOURCE_DESC_HPP

#include <string>
#include <rttr/type>
#include "../../editor/editor_document.hpp"

typedef EditorDocument*(*create_resource_doc_fn_t)(ResourceNode* node);

struct ResourceDesc {
    rttr::type type;
    create_resource_doc_fn_t create_resource_doc_fn;
};

#endif
