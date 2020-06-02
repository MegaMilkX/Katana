#ifndef ECS_ARCHETYPE_HPP
#define ECS_ARCHETYPE_HPP

#include <tuple>

#include "entity.hpp"

template<typename T>
struct ecsExclude {};
template<typename T>
struct ecsOptional {};

class ecsTupleBase {
protected:
    entity_id entity_uid;
public:
    virtual ~ecsTupleBase() {}
    virtual uint64_t get_signature() const = 0;
    virtual uint64_t get_optional_signature() const = 0;
    virtual uint64_t get_exclusion_signature() const = 0;

    virtual void init(ecsWorld* world, entity_id ent) = 0;
    virtual void updateOptionals(ecsWorld* world, entity_id ent) = 0;
    virtual void clearOptionals(uint64_t mask) = 0;

    entity_id getEntityUid() const { return entity_uid; }

    virtual void signalAttribUpdate(uint64_t attrib_sig) = 0;
};
template<typename Arg>
class ecsTuplePart {
protected:
    Arg* ptr;
public:
    ecsTuplePart() {}
    ecsTuplePart(ecsWorld* world, entity_id ent) {
        ptr = world->findAttrib<Arg>(ent);
    }
    virtual ~ecsTuplePart() {}

    static uint64_t get_inclusion_sig() {
        return 1 << Arg::get_id_static();
    }
    static uint64_t get_optional_sig() {
        return 0;
    }
    static uint64_t get_exclusion_sig() {
        return 0;
    }

    int updateOptional(ecsWorld* world, entity_id ent) { return 0; }
    int clearOptional(uint64_t mask) { return 0; }

    int _signalAttribUpdate(uint64_t attrib_sig) {
        if((1 << Arg::get_id_static()) == attrib_sig) {
            onAttribUpdate(ptr);
        }
        return 0;
    }

    virtual void onAttribUpdate(Arg* arg) {}
};
template<typename Arg>
class ecsTuplePart<ecsExclude<Arg>> {
public:
    ecsTuplePart() {}
    ecsTuplePart(ecsWorld* world, entity_id ent) {}
    virtual ~ecsTuplePart() {}

    static uint64_t get_inclusion_sig() {
        return 0;
    }
    static uint64_t get_optional_sig() {
        return 0;
    }
    static uint64_t get_exclusion_sig() {
        return 1 << Arg::get_id_static();
    }

    int updateOptional(ecsWorld* world, entity_id ent) { return 0; }
    int clearOptional(uint64_t mask) { return 0; }

    int _signalAttribUpdate(uint64_t attrib_sig) {
        return 0;
    }
};
template<typename Arg>
class ecsTuplePart<ecsOptional<Arg>> {
protected:
    Arg* ptr = 0;
public:
    ecsTuplePart() {}
    ecsTuplePart(ecsWorld* world, entity_id ent) {
        //ptr = ent->findAttrib<Arg>();
    }
    virtual ~ecsTuplePart() {}
    
    static uint64_t get_inclusion_sig() {
        return 0;
    }
    static uint64_t get_optional_sig() {
        return 1 << Arg::get_id_static();
    }
    static uint64_t get_exclusion_sig() {
        return 0;
    }

    int updateOptional(ecsWorld* world, entity_id ent) {
        Arg* tmp = world->findAttrib<Arg>(ent);
        if(tmp && !ptr) {
            ptr = tmp;
            onAddOptional(ptr);
        } else if(!tmp && ptr) {
            ptr = tmp;
            onRemoveOptional(ptr);
        }
        return 0;
    }
    int clearOptional(uint64_t mask) {
        if(ptr && ((1 << Arg::get_id_static()) & mask)) {
            onRemoveOptional(ptr);
            ptr = 0;
        }
        return 0;
    }

    int _signalAttribUpdate(uint64_t attrib_sig) {
        if((1 << Arg::get_id_static()) == attrib_sig) {
            onAttribUpdate(ptr);
        }
        return 0;
    }

    virtual void onAttribUpdate(Arg* arg) {}
    virtual void onAddOptional(Arg* arg) {}
    virtual void onRemoveOptional(Arg* arg) {}
};

template<typename... Args>
class ecsTuple : public ecsTupleBase, public ecsTuplePart<Args>... {
public:
    ecsTuple() {}
    ecsTuple(ecsWorld* world, entity_id ent)
    : ecsTuplePart<Args>(world, ent)... {}

    void init(ecsWorld* world, entity_id ent) override {
        *this = ecsTuple<Args...>(world, ent);
        entity_uid = ent;
    }

    void updateOptionals(ecsWorld* world, entity_id ent) override {
        int x[] = { ecsTuplePart<Args>::updateOptional(world, ent)... };
    }
    void clearOptionals(uint64_t mask) override {
        int x[] = { ecsTuplePart<Args>::clearOptional(mask)... };
    }

    template<typename T>
    T* get() {
        return ecsTuplePart<T>::ptr;
        //return std::get<T*>(attribs);
    }
    template<typename T>
    T* get_optional() {
        return ecsTuplePart<ecsOptional<T>>::ptr;
    }

    void signalAttribUpdate(uint64_t attrib_sig) {
        int x[] = { ecsTuplePart<Args>::_signalAttribUpdate(attrib_sig)... };
    }

    static uint64_t get_signature_static() {
        static uint64_t x[] = { ecsTuplePart<Args>::get_inclusion_sig()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_signature() const override {
        return get_signature_static();
    }

    static uint64_t get_optional_signature_static() {
        static uint64_t x[] = { ecsTuplePart<Args>::get_optional_sig()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_optional_signature() const override {
        return get_optional_signature_static();
    }

    static uint64_t get_exclusion_signature_static() {
        static uint64_t x[] = { ecsTuplePart<Args>::get_exclusion_sig()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_exclusion_signature() const override {
        return get_exclusion_signature_static();
    }
};

#endif
