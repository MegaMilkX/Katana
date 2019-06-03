#include "constraint_stack.hpp"

#include "../scene/controllers/constraint_ctrl.hpp"

STATIC_RUN(ConstraintStack) {
    rttr::registration::class_<ConstraintStack>("ConstraintStack")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

ConstraintStack::~ConstraintStack() {
}
void ConstraintStack::onCreate() {
    getOwner()->getScene()->getController<ConstraintCtrl>();
}