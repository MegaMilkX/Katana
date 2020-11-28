#ifndef RESOURCE_2_HPP
#define RESOURCE_2_HPP

#include <string>


// Load binary data, no deserialization
void resCache(const char* path);

template<typename T>
T* resCreate();


#endif
