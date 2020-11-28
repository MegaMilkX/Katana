#include "mesh_gen.hpp"

#include <vector>
#include "../gfxm.hpp"

void makeSphere(gl::IndexedMesh* mesh, float radius, int segments) {
    if(segments < 1) segments = 1;
    int sectorCount = 3 + segments;
    int stackCount = 1 + segments;

    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;

    float sectorStep = 2 * gfxm::pi / sectorCount;
    float stackStep = gfxm::pi / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i) {
        stackAngle = gfxm::pi / 2 - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            vertices.emplace_back(gfxm::vec3(x, y, z));
        }
    }

    int k1 = 0, k2 = 0;
    for(int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if(i != 0) {
                indices.emplace_back(k1);
                indices.emplace_back(k2);
                indices.emplace_back(k1 + 1);
            }

            if(i != (stackCount - 1)) {
                indices.emplace_back(k1 + 1);
                indices.emplace_back(k2);
                indices.emplace_back(k2 + 1);
            }
        }
    }

    mesh->setAttribData(VFMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(gfxm::vec3));
    mesh->setIndices(indices.data(), indices.size());
}


void makeCylinder();
void makeCone();
void makeCapsule();
void makeBox();
