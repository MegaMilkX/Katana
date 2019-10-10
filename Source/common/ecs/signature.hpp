#ifndef ECS_SIGNATURE_HPP
#define ECS_SIGNATURE_HPP

class ktEcsSignature {
public:
    template<typename T>
    ktEcsSignature& require();
    template<typename T>
    ktEcsSignature& optional();
};

#endif
