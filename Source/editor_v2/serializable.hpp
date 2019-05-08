#ifndef SERIALIZABLE_HPP
#define SERIALIZABLE_HPP

#include "../common/util/data_stream.hpp"

class Serializable {
public:
    virtual ~Serializable() {}
    virtual void serialize(out_stream& out) = 0;
    virtual void deserialize(in_stream& in) = 0;
};

#endif
