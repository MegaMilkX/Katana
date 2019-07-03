#ifndef SERIALIZABLE_HPP
#define SERIALIZABLE_HPP

#include "../common/util/data_stream.hpp"

class Serializable {
public:
    virtual ~Serializable() {}
    virtual void write(out_stream& out) = 0;
    virtual void read(in_stream& in) = 0;
};

#endif
