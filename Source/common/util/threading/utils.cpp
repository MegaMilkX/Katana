#include "utils.hpp"


static std::thread::id main_thread_id;

void setMainThread(std::thread::id id) {
    main_thread_id = id;
}
bool isMainThread() {
    return std::this_thread::get_id() == main_thread_id;
}