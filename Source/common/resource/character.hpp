#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "resource.h"

#include "model/model.hpp"
#include "motion.hpp"

class Character : public Resource {
    RTTR_ENABLE(Resource)

    std::shared_ptr<Model_> model;
    std::shared_ptr<Motion> motion;
    // TODO: Add collision?

public:

    void serialize(out_stream& out) override {
        
    }
    bool deserialize(in_stream& in, size_t sz) override {
        return true;
    }
};
STATIC_RUN(Character) {
    rttr::registration::class_<Character>("Character")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

#endif
