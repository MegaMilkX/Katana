#ifndef HANDLE_MGR_H
#define HANDLE_MGR_H

#include <vector>
#include <stdint.h>
#include <assert.h>

template<typename TAG>
class Handle {
    union {
        enum {
            MAX_BITS_INDEX = 16,
            MAX_BITS_MAGIC = 16,
            MAX_INDEX = (1 << MAX_BITS_INDEX) - 1,
            MAX_MAGIC = (1 << MAX_BITS_MAGIC) - 1
        };
        struct {
            uint32_t m_Index : MAX_BITS_INDEX;
            uint32_t m_Magic : MAX_BITS_MAGIC;
        };
        uint32_t m_Handle;
    };
public:
    Handle()
    : m_Handle(0) {}

    void init(uint32_t index);

    uint32_t getIndex() const { return m_Index; }
    uint32_t getMagic() const { return m_Magic; }
    uint32_t getHandle() const { return m_Handle; }
    bool isNull() const { return !m_Handle; }

    operator uint32_t() const { return m_Handle; }
};

template<typename TAG>
void Handle<TAG>::init(uint32_t index) {
    assert(isNull());
    assert(index <= MAX_INDEX);
    static uint32_t s_AutoMagic = 0;
    if(++s_AutoMagic > MAX_MAGIC) {
        s_AutoMagic = 1;
    }
    m_Index = index;
    m_Magic = s_AutoMagic;
}

template<typename TAG>
inline bool operator!=(Handle<TAG> l, Handle<TAG> r) {
    return l.getHandle() != r.getHandle();
}
template<typename TAG>
inline bool operator==(Handle<TAG> l, Handle<TAG> r) {
    return l.getHandle() == r.getHandle();
}

template<typename DATA, typename HANDLE>
class HandleMgr {
public:
    HandleMgr() {}
    ~HandleMgr() {}

    DATA* acquire(HANDLE& handle) {
        unsigned index;
        if(m_FreeSlots.empty()) {
            index = m_MagicNumbers.size();
            handle.init(index);
            m_UserData.emplace_back(DATA());
            m_MagicNumbers.emplace_back(handle.getMagic());
        } else {
            index = m_FreeSlots.back();
            handle.init(index);
            m_FreeSlots.pop_back();
            m_MagicNumbers[index] = handle.getMagic();
        }
        return m_UserData.begin() + index;
    }
    void release(HANDLE handle) {
        uint32_t index = handle.getIndex();

        assert(index < m_UserData.size());
        assert(m_MagicNumbers[index] == handle.getMagic());

        m_MagicNumbers[index] = 0;
        m_FreeSlots.emplace_back(index);
    }

    DATA* dereference(HANDLE handle) {
        if(handle.isNull()) return 0;

        uint32_t index = handle.getIndex();
        if((index >= m_UserData.size()) || (m_MagicNumbers[index] != handle.getMagic())) {
            assert(0);
            return 0;
        }
        return m_UserData.begin() + index;
    }
    const DATA* dereference(HANDLE handle) const {
        typedef HandleMgr<DATA, HANDLE> this_type;
        return const_cast<this_type*>(this)->dereference(handle);
    }

    unsigned int getUsedHandleCount() const {
        return m_MagicNumbers.size() - m_FreeSlots.size();
    }
    bool hasUsedHandles() const {
        return !!getUsedHandleCount();
    }

private:
    typedef std::vector<DATA> user_vec_t;
    typedef std::vector<uint32_t> magic_vec_t;
    typedef std::vector<uint32_t> free_vec_t;

    user_vec_t m_UserData;
    magic_vec_t m_MagicNumbers;
    free_vec_t m_FreeSlots;
};

#endif
