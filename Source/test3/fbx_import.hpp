#ifndef FBX_IMPORT_HPP
#define FBX_IMPORT_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "scene_object.hpp"

bool importFbx(const std::string& fname, SceneObject* o);

#endif
