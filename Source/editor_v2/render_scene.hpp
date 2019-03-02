#ifndef RENDER_SCENE_HPP
#define RENDER_SCENE_HPP

#include "render_viewport.hpp"
#include "../common/gfxm.hpp"
#include "../common/gl/indexed_mesh.hpp"

#include <set>

struct RenderObject {
    gl::IndexedMesh* mesh;
    gfxm::mat4 transform;
};

class RenderScene {
public:
    RenderObject* createObject(gl::IndexedMesh* mesh) {
        auto ro = new RenderObject{mesh};
        objects.insert(ro);
    }
    void removeObject(RenderObject* ro) {
        delete ro;
        objects.erase(ro);
    }

    void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {

    }
private:
    std::set<RenderObject*> objects;
};

#endif
