#ifndef MESH_GEN_HPP
#define MESH_GEN_HPP

#include <vector>
#include "../gfxm.hpp"
#include "../gl/indexed_mesh.hpp"

void makeSphere(gl::IndexedMesh* mesh, float radius, int segments);
void makeCylinder();
void makeCone();
void makeCapsule();
void makeBox();

#endif
