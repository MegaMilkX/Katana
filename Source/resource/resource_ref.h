#ifndef RESOURCE_REF_H
#define RESOURCE_REF_H

#include <memory>
#include <rttr/type>

#include "resource_factory.h"

class i_resource_ref {
    RTTR_ENABLE()
public:
    virtual ~i_resource_ref() {}
    virtual std::shared_ptr<Resource> set_from_factory(ResourceFactory& factory, const std::string& name) = 0;
    virtual std::shared_ptr<Resource> set_from_data(DataSourceRef& data) = 0;
    virtual void set_unsafe(std::shared_ptr<Resource>& resource) = 0;
    virtual std::shared_ptr<Resource> base_ptr() = 0;
};

template<typename T>
class resource_ref : public i_resource_ref {
    RTTR_ENABLE(i_resource_ref)
public:
    resource_ref() {}
    resource_ref(T* r)
    : ptr(r) {}
    resource_ref(const std::shared_ptr<T>& r)
    : ptr(r) {}

    virtual std::shared_ptr<Resource> set_from_factory(ResourceFactory& factory, const std::string& name) {
        ptr = factory.Get<T>(name);
        return dynamic_pointer_cast<Resource>(ptr);
    }
    virtual std::shared_ptr<Resource> set_from_data(DataSourceRef& data) {
        std::cout << "Building a resource from data" << std::endl;
        if(!data) {
            std::cout << "data ref is null" << std::endl;
        }
        std::shared_ptr<T> p(new T());
        if(!p->Build(data)) {
            std::cout << "Failed to build resource from data" << std::endl;
        }
        p->Storage(Resource::LOCAL);
        ptr = p;
        return dynamic_pointer_cast<Resource>(p);
    }
    virtual void set_unsafe(std::shared_ptr<Resource>& resource) {
        ptr = dynamic_pointer_cast<T>(resource);
    }

    void reset(T* p) { ptr.reset(p); }
    void swap(resource_ref& r) { ptr.swap(r.ptr); }
    T* get() { return ptr.get(); }
    const T* get() const { return ptr.get(); }
    T& operator*() { return *ptr; }
    const T& operator*() const { return *ptr; }
    T* operator->() { return ptr.operator->(); }
    const T* operator->() const { return ptr.operator->(); }
    // TODO: operator[]
    long use_count() const { return ptr.use_count(); }
    virtual operator bool() { return ptr.operator bool(); }
    // TODO: owner_before

    operator std::shared_ptr<T>() {
        return ptr;
    }
    virtual std::shared_ptr<Resource> base_ptr() {
        return dynamic_pointer_cast<Resource>(ptr);
    }

    template<typename U>
    resource_ref<U> dynamic_ptr_cast() noexcept {
        resource_ref<U> ref;
        ref.ptr = dynamic_pointer_cast<U>(ptr);
        return ref;
    }
private:
    std::shared_ptr<T> ptr;
};

template<class T, class U>
resource_ref<T> dynamic_pointer_cast(const resource_ref<U>& r) noexcept {
    return r.dynamic_ptr_cast<T>();
}

namespace rttr {

template<typename T>
struct wrapper_mapper<resource_ref<T>>
{
    using wrapped_type = decltype(std::declval<resource_ref<T>>().get());
    using type = resource_ref<T>;

    inline static wrapped_type get(const type& obj) {
        return (wrapped_type)obj.get();
    }
    inline static type create(const wrapped_type& value) {
        return resource_ref<T>(value);
    }
    template<typename U>
    inline static resource_ref<U> convert(const type& source, bool& ok) {
        if(auto obj = rttr_cast<typename resource_ref<U>::wrapped_type*>(&source.get())) {
            ok = true;
            return resource_ref<U>(*obj);
        } else {
            ok = false;
            return resource_ref<U>();
        }
    }
};

}

#endif
