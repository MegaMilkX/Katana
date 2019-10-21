#ifndef BITSET_HPP
#define BITSET_HPP

#include <vector>
#include <algorithm>
#undef max
#undef min

class bitset {
    std::vector<char> bytes;
public:
    size_t bitCount() const {
        const int bits_per_byte = 8;
        return bytes.size() * bits_per_byte;
    }
    size_t enabledBitCount() const {
        size_t count = 0;
        for(size_t i = 0; i < bitCount(); ++i) {
            if(test(i)) {
                ++count;
            }
        }
        return count;
    }

    void clear() {
        bytes.clear();
    }

    void resize(size_t bytes_) {
        bytes.resize(bytes_);
    }

    bitset operator&(const bitset& other) const {
        bitset bits;
        bits.resize(std::min(bytes.size(), other.bytes.size()));
        for(size_t i = 0; i < bits.bytes.size(); ++i) {
            bits.bytes[i] = bytes[i] & other.bytes[i];
        }
        return bits;
    }
    bool operator==(const bitset& other) const {
        for(size_t i = 0; i < std::min(bytes.size(), other.bytes.size()); ++i) {
            if(bytes[i] != other.bytes[i]) {
                return false;
            }
        }
        return true;
    }

    void set(size_t bit, bool value) {
        const int bits_per_byte = 8;
        int byte_id = bit / bits_per_byte;
        bit = bit - byte_id * bits_per_byte;
        if(bytes.size() <= byte_id) {
            resize(byte_id + 1);
        }
        if(value) {
            bytes[byte_id] |= (1 << bit);
        } else {
            bytes[byte_id] &= ~(1 << bit);
        }
    }

    bool test(size_t bit) const {
        const int bits_per_byte = 8;
        int byte_id = bit / bits_per_byte;
        bit = bit - byte_id * bits_per_byte;
        if(bytes.size() <= byte_id) {
            return false;
        }
        return bytes[byte_id] & (1 << bit);
    }

    size_t hash() const {
        size_t ret = 0;
        for(size_t i = 0; i < bytes.size(); ++i) {
            ret = ret * 31 + std::hash<char>()(bytes[i]);
        }
        return ret;
    }
};

namespace std {
template <>
struct hash<::bitset> {
    std::size_t operator()(const ::bitset& k) const {
        return k.hash();
    }
};
}

#endif
