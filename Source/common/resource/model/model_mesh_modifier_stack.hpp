#ifndef MODEL_MESH_MODIFIER_STACK_HPP
#define MODEL_MESH_MODIFIER_STACK_HPP

#include "model_mesh_modifiers/model_mesh_modifier.hpp"

#include <memory>
#include "../../gl/vertex_array_object.hpp"


class ModelMeshModifierStack {
    std::shared_ptr<gl::VertexArrayObject>  vao;
    
};

#endif
