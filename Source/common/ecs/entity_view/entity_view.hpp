#ifndef ECS_ENTITY_VIEW_HPP
#define ECS_ENTITY_VIEW_HPP

#include "../attribs/base_attribs.hpp"

template<typename... Args>
uint64_t ecsMakeAttribSignature() {
    uint64_t signature = 0;
    attrib_id indices[] = { Args::get_id_static()... };
    for(int i = 0; i < sizeof...(Args); ++i) {
        auto index = indices[i];
        signature |= 1ULL << index;
    }
    return signature;
}

class ecsEntityViewBase {
public:
};

template<typename VIEW_T, typename... Args>
class ecsEntityView {
public:
    static const uint64_t signature;
    ArchetypeStorage* storage = 0;
    std::vector<uint64_t> attrib_indices;
    std::vector<ecsAttribBase**> attrib_pointers;

    ecsEntityView() {}

    static uint64_t get_signature() {
        return signature;
    }

    void initAttribPointers(Args**... args) {
        attrib_indices = { Args::get_id_static()... };
        attrib_pointers = { ((ecsAttribBase**)args)... };
    }
};
template<typename VIEW_T, typename... Args>
const uint64_t ecsEntityView<VIEW_T, Args...>::signature = ecsMakeAttribSignature<Args...>();


class ecsEntityViewTest : public ecsEntityView<ecsEntityViewTest, ecsName, ecsLightOmni, ecsTranslation> {
public:
    ecsName* name;
    ecsLightOmni* light;
    ecsTranslation* translation;
    ecsEntityViewTest() {
        initAttribPointers(&name, &light, &translation);
    }
};


#endif
