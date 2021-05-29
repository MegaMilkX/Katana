#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"
#include "../storage/storage_transform.hpp"


class ecsysSceneGraph;
class ecsTupleSubGraph : public ecsTuple<
    ecsSubScene,
    ecsOptional<ecsWorldTransform>
> {
public:
    ecsysSceneGraph* sub_system = 0;

    void onAttribUpdate(ecsSubScene* ss) {
        if(ss->getWorld()) {
            sub_system = ss->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }
};

class ecsTplConstraint : public ecsTuple<
    ecsOptional<ecsTranslation>, ecsOptional<ecsRotation>, ecsConstraint
> {
public:
};
class ecsTplIK : public ecsTuple<ecsWorldTransform, ecsIK> {};

class ecsTplModel : public ecsTuple<ecsWorldTransform, ecsModel>{};

class ecsysSceneGraph : public ecsSystem<
    tupleTransform,
    ecsTplModel,
    ecsTupleSubGraph,
    ecsTplConstraint,
    ecsTplIK
> {
public:
    gfxm::mat4 root_transform = gfxm::mat4(1.0f);

    void onFit(ecsTupleSubGraph* sub_graph) {
        if(sub_graph->get<ecsSubScene>()->getWorld()) {
            sub_graph->sub_system = sub_graph->get<ecsSubScene>()->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }

    void updateDirtyTransforms() {
        sort_dirty_array<tupleTransform>();
        for(int i = get_dirty_index<tupleTransform>(); i < count<tupleTransform>(); ++i) {
            auto tuple = get<tupleTransform>(i);
            ecsTranslation* translation     = tuple->get_optional<ecsTranslation>();
            ecsRotation* rotation           = tuple->get_optional<ecsRotation>();
            ecsScale* scale                 = tuple->get_optional<ecsScale>();
            ecsWorldTransform* world        = tuple->get<ecsWorldTransform>();

            gfxm::mat4 local = gfxm::mat4(1.0f);
            if(translation) {
                local = gfxm::translate(local, translation->getPosition());
            }
            if(rotation) {
                local = local * gfxm::to_mat4(rotation->getRotation());
            }
            if(scale) {
                local = gfxm::scale(local, scale->getScale());
            }
            world->setTransform(local);

            if(tuple->get_parent()) {
                world->setTransform(tuple->get_parent()->get<ecsWorldTransform>()->getTransform() * world->getTransform());
            } else {
                world->setTransform(root_transform * world->getTransform());
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<tupleTransform>();

        for(auto& a : get_array<ecsTupleSubGraph>()) {
            if(!a->sub_system) continue;
            auto opt_world = a->get_optional<ecsWorldTransform>();
            if(opt_world) {
                a->sub_system->root_transform = opt_world->getTransform();
                a->sub_system->set_dirty_index<tupleTransform>(0);
            }
            a->sub_system->onUpdate(1.0f/60.0f);
        }
    }

    void onUpdate(float dt) {
        updateDirtyTransforms();

        for(auto& a : get_array<ecsTplConstraint>()) {
            auto con = a->get<ecsConstraint>();
            auto t = a->get_optional<ecsTranslation>();
            auto r = a->get_optional<ecsRotation>();
            auto& tgt = con->getTarget();
            if(!tgt.isValid()) {
                continue;
            }
            auto tgt_w = tgt.findAttrib<ecsWorldTransform>();
            if(!tgt_w) {
                continue;
            }

            if(t) {
                t->setPosition(tgt_w->getTransform() * gfxm::vec4(0,0,0,1));
            }
            if(r) {
                r->setRotation(gfxm::to_quat(gfxm::to_orient_mat3(tgt_w->getTransform())));
            }
        }

        for(auto& a : get_array<ecsTplIK>()) {
            auto w = a->get<ecsWorldTransform>();
            ecsEntityHandle bone_a = w->getEntityHdl().getParent();
            if(!bone_a.isValid()) continue;
            ecsEntityHandle bone_b = bone_a.getParent();
            if(!bone_b.isValid()) continue;
            auto r_a = bone_a.findAttrib<ecsRotation>();
            auto r_b = bone_b.findAttrib<ecsRotation>();
            auto w_a = bone_a.findAttrib<ecsWorldTransform>();
            auto w_b = bone_b.findAttrib<ecsWorldTransform>();
            if(!r_a || !r_b || !w_b || !w_a) continue;

            {
                float eps = 0.01f;
                gfxm::vec3 t = gfxm::vec3(0, 1, 0);
                gfxm::vec3 c   = w->getTransform() * gfxm::vec4(0,0,0,1);
                gfxm::vec3 b  = w_a->getTransform() * gfxm::vec4(0,0,0,1);
                gfxm::vec3 a = w_b->getTransform() * gfxm::vec4(0,0,0,1);
                gfxm::mat4 b_w = w_a->getTransform();
                gfxm::mat4 a_w = w_b->getTransform();
                float lab = gfxm::length(b - a);
                float lcb = gfxm::length(b - c);
                float lat = gfxm::clamp(gfxm::length(t - a), eps, lab + lcb - eps);

                float ac_ab_0 = acosf(gfxm::clamp(
                    gfxm::dot(gfxm::normalize(c - a), gfxm::normalize(b - a)),
                    -1, 1
                ));
                float ba_bc_0 = acosf(gfxm::clamp(
                    gfxm::dot(gfxm::normalize(a - b), gfxm::normalize(c - b)),
                    -1, 1
                ));

                float ac_ab_1 = acosf(gfxm::clamp((lcb*lcb - lab*lab - lat*lat) / (-2*lab*lat), -1, 1));
                float ba_bc_1 = acosf(gfxm::clamp((lat*lat - lab*lab - lcb*lcb) / (-2*lab*lcb), -1, 1));

                gfxm::vec3 axis0 = gfxm::normalize(gfxm::cross(c - a, b - a));
                gfxm::quat r0    = gfxm::angle_axis(ac_ab_1 - ac_ab_0, (gfxm::vec3)(gfxm::inverse(a_w) * gfxm::vec4(axis0, .0f)));
                gfxm::quat r1    = gfxm::angle_axis(ba_bc_1 - ba_bc_0, (gfxm::vec3)(gfxm::inverse(b_w) * gfxm::vec4(axis0, .0f)));

                r_a->setRotation(r_a->getRotation() * r1);
                r_b->setRotation(r_b->getRotation() * r0);

                float ac_at_0 = acosf(gfxm::clamp(gfxm::dot(gfxm::normalize(c - a), gfxm::normalize(t - a)), -1, 1));
                gfxm::vec3 axis1 = gfxm::normalize(gfxm::cross(c - a, t - a));
                gfxm::quat r2 = gfxm::angle_axis(ac_at_0, (gfxm::vec3)(gfxm::inverse(a_w) * gfxm::vec4(axis1, .0f)));

                r_b->setRotation(r_b->getRotation() * r2);
            }

            
        }

        updateDirtyTransforms(); // Upd again to fix objects affected by constraints

        // Update model roots
        for(int i = get_dirty_index<ecsTplModel>(); i < count<ecsTplModel>(); ++i) {
            auto tuple = get<ecsTplModel>(i);
            auto mdl  = tuple->get<ecsModel>();
            if(mdl->model) {
                // TODO:
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<ecsTplModel>();
    }

};


#endif
