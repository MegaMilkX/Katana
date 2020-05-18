#ifndef THREADING_UTILS_HPP
#define THREADING_UTILS_HPP

#include <thread>

void setMainThread(std::thread::id id);
bool isMainThread();


#endif
