#ifndef ECS_ARCHETYPE_HPP
#define ECS_ARCHETYPE_HPP

#include <tuple>
#include <rttr/type>

class ktEcsWorld;
class ktEntity;
template<typename... Args>
class ktEcsArchetype {
public:
    std::tuple<Args...> attribs;

    static bool requiresAttrib(rttr::type t) {
        static const rttr::type types[] = { rttr::type::get<Args>()... };
        for(size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
            if(types[i] == t) {
                return true;
            }
        }
        return false;
    }
    static bool entityFits(ktEcsWorld& world, ktEntity ent) {
        return world.hasAttribs<Args...>(ent);
    }
};

#endif
