#ifndef BULLET_DEBUG_DRAW_HPP
#define BULLET_DEBUG_DRAW_HPP

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <bulletcollision/collisiondispatch/btghostobject.h>
#include "../../common/gfxm.hpp"
#include "../../common/debug_draw.hpp"

class BulletDebugDrawer2_OpenGL : public btIDebugDraw {
public:
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) 
	{
        if(dd) {
            dd->line(*(gfxm::vec3*)&from, *(gfxm::vec3*)&to, *(gfxm::vec3*)&color);
        }
	}
	virtual void drawContactPoint(const btVector3 &, const btVector3 &, btScalar, int, const btVector3 &) {}
	virtual void reportErrorWarning(const char * e) {
        LOG_WARN(e);
    }
	virtual void draw3dText(const btVector3 & pos, const char * text) {
        
    }
	virtual void setDebugMode(int p) {
		m = p;
	}
	int getDebugMode(void) const { return DBG_DrawWireframe /*| DBG_DrawAabb*/ | DBG_DrawContactPoints; }
	int m;

    void setDD(DebugDraw* dd) {
        this->dd = dd;
    }
private:
    DebugDraw* dd = 0;
};

#endif
