#ifndef UTIL_IMPORT_ASSIMP_HPP
#define UTIL_IMPORT_ASSIMP_HPP

#include "../assimp_scene.hpp"
#include "../../resource/mesh.hpp"
#include "../../resource/texture2d.h"
#include "../../resource/material.hpp"
#include "../../resource/animation.hpp"


std::shared_ptr<Mesh> assimpMergeMeshes(const std::vector<const aiMesh*>& ai_meshes);


#endif
