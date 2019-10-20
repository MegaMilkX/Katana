#ifndef BITSET_HPP
#define BITSET_HPP

#include <vector>

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
        return count;//attribs.count();
    }

    void clear() {
        bytes.clear();
    }

    void resize(size_t sz) {
        bytes.resize(sz);
    }

    void set(size_t bit, bool value) {
        const int bits_per_byte = 8;
        int byte_id = bit / bits_per_byte;
        bit = bit - byte_id * bits_per_byte;
        if(bytes.size() <= byte_id) {
            resize(byte_id + 1);
        }
        bytes[byte_id] |= (1 << bit);
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
};

#endif
