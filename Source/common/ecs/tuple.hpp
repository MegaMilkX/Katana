#ifndef ECS_ARCHETYPE_HPP
#define ECS_ARCHETYPE_HPP

#include <tuple>

#include "entity.hpp"

template<typename T>
struct ecsExclude {};
template<typename T>
struct ecsOptional {};

#include "tuple_base.hpp"

template<typename Arg>
class ecsTuplePart {
protected:
public:
    ecsTuplePart() {}
    ecsTuplePart(ecsWorld* world, entity_id ent) {
        ptr = world->findAttrib<Arg>(ent);
    }
    virtual ~ecsTuplePart() {}

    static uint64_t get_inclusion_sig() {
        return 1ULL << Arg::get_id_static();
    }
    static uint64_t get_optional_sig() {
        return 0;
    }
    static uint64_t get_exclusion_sig() {
        return 0;
    }

    int updateOptional(ecsWorld* world, entity_id ent) { return 0; }
    int clearOptional(ecsEntityHandle hdl, uint64_t mask) { return 0; }

    int _signalAttribUpdate(ecsEntityHandle hdl, uint64_t attrib_sig) {
        if((1ULL << Arg::get_id_static()) == attrib_sig) {
            onAttribUpdate(hdl.findAttrib<Arg>());
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
        return 1ULL << Arg::get_id_static();
    }

    int updateOptional(ecsWorld* world, entity_id ent) { return 0; }
    int clearOptional(ecsEntityHandle hdl, uint64_t mask) { return 0; }

    int _signalAttribUpdate(ecsEntityHandle hdl, uint64_t attrib_sig) {
        return 0;
    }
};
template<typename Arg>
class ecsTuplePart<ecsOptional<Arg>> {
protected:
    Arg* stale_ptr = 0; // To check for add/remove events
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
        return 1ULL << Arg::get_id_static();
    }
    static uint64_t get_exclusion_sig() {
        return 0;
    }

    int updateOptional(ecsWorld* world, entity_id ent) {
        Arg* tmp = world->findAttrib<Arg>(ent);
        if(tmp && !stale_ptr) {
            stale_ptr = tmp;
            onAddOptional(stale_ptr);
        } else if(!tmp && stale_ptr) {
            stale_ptr = tmp;
            onRemoveOptional(stale_ptr);
        }
        return 0;
    }
    int clearOptional(ecsEntityHandle hdl, uint64_t mask) {
        if(((1ULL << Arg::get_id_static()) & mask)) {
            onRemoveOptional(hdl.findAttrib<Arg>());
        }
        return 0;
    }

    int _signalAttribUpdate(ecsEntityHandle hdl, uint64_t attrib_sig) {
        if((1ULL << Arg::get_id_static()) == attrib_sig) {
            onAttribUpdate(hdl.findAttrib<Arg>());
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
        hdl = ecsEntityHandle(world, ent);
    }

    void updateOptionals(ecsWorld* world, entity_id ent) override {
        int x[] = { ecsTuplePart<Args>::updateOptional(world, ent)... };
    }
    void clearOptionals(uint64_t mask) override {
        int x[] = { ecsTuplePart<Args>::clearOptional(hdl, mask)... };
    }

    template<typename T>
    T* get() {
        return hdl.findAttrib<T>();
    }
    template<typename T>
    T* get_optional() {
        return hdl.findAttrib<T>();
    }

    ecsTuple<Args...>* get_parent() {
        return (ecsTuple<Args...>*)parent;
    }
    ecsTuple<Args...>* get_next_sibling() {
        return (ecsTuple<Args...>*)next_sibling;
    }
    ecsTuple<Args...>* get_first_child() {
        return (ecsTuple<Args...>*)first_child;
    }

    void signalAttribUpdate(uint64_t attrib_sig) {
        int x[] = { ecsTuplePart<Args>::_signalAttribUpdate(hdl, attrib_sig)... };
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
