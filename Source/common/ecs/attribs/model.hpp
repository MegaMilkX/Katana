#ifndef ECS_MODEL_HPP
#define ECS_MODEL_HPP

#include "../attribute.hpp"
#include "../../util/imgui_helpers.hpp"

#include "../../resource/model/model.hpp"


class ecsModel : public ecsAttrib<ecsModel> {
public:
    std::shared_ptr<Model_> model;
};


#endif
