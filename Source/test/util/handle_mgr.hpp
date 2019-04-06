#ifndef HANDLE_MGR_HPP
#define HANDLE_MGR_HPP

#include <stdint.h>
#include <vector>
#include <set>

class handle_ {
public:
    handle_()
    : hdl(0) {}
    handle_(uint32_t index, uint32_t magic) {
        this->index = index;
        this->magic = magic;
    }
    handle_(const uint64_t& hdl) {
        this->hdl = hdl;
    }
    handle_& operator=(const uint64_t& hdl) {
        this->hdl = hdl;
    }
    operator uint64_t() const {
        return hdl;
    }
    uint32_t getIndex() const {
        return index;
    }
    uint32_t getMagic() const {
        return magic;
    }
private:
    union {
        uint64_t hdl;
        struct {
            uint32_t index;
            uint32_t magic;
        };
    };
};

class handle_mgr_base {
public:
    virtual ~handle_mgr_base() {}
    virtual handle_    acquire() = 0;
    virtual void*       dereference(handle_) = 0;
    virtual const void* dereference(handle_) const = 0;
    virtual void        free(handle_) = 0;
    virtual uint32_t    count() const = 0;
};
template<typename T>
class handle_mgr : public handle_mgr_base {
public:
    handle_      acquire();
    T*          deref(handle_);
    const T*    deref(handle_) const;
    void        free(handle_);
    uint32_t    count() const;
private:
    void*       dereference(handle_);
    const void* dereference(handle_) const;

    std::vector<T>          data;
    std::vector<uint32_t>   magic;
    std::set<uint32_t>      free_slots;
    uint32_t                _count;
};

template<typename T>
handle_ handle_mgr<T>::acquire() {
    if(!free_slots.empty()) {
        uint32_t slot = *free_slots.begin();
        free_slots.erase(free_slots.begin());
        handle_ h(slot, ++magic[slot]);
        _count++;
        return h;
    } else {
        data.resize(data.size() + 1);
        magic.resize(data.size());
        handle_ h(data.size() - 1, ++magic[data.size() - 1]);
        _count++;
        return h;
    }
}
template<typename T>
void* handle_mgr<T>::dereference(handle_ h) {
    if(h.getMagic() != magic[h.getIndex()]) {
        return 0;
    }
    if(h.getIndex() >= data.size()) {
        return 0;
    }
    return &data[h.getIndex()];
}
template<typename T>
const void* handle_mgr<T>::dereference(handle_ h) const {
    return deref(h);
}
template<typename T>
T* handle_mgr<T>::deref(handle_ h) {
    return (T*)dereference(h);
}
template<typename T>
const T* handle_mgr<T>::deref(handle_ h) const {
    return (T*)dereference(h);
}
template<typename T>
void handle_mgr<T>::free(handle_ h) {
    free_slots.insert(h.getIndex());
    _count--;
}
template<typename T>
uint32_t handle_mgr<T>::count() const {
    return _count;
}

// ====================

template<typename T>
handle_mgr_base& get_handle_mgr_impl() {
    static handle_mgr<T> mgr;
    return mgr;
}
template<typename T>
handle_mgr<T>& get_handle_mgr() {
    return (handle_mgr<T>&)get_handle_mgr_impl<T>();
}

#endif
