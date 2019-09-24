#ifndef THUMB_BUILDER_HPP
#define THUMB_BUILDER_HPP

#include "../common/util/singleton.hpp"

#include <string>
#include <thread>
#include <mutex>
#include <queue>

class ThumbBuilder : public Singleton<ThumbBuilder> {
    bool working = false;
    std::thread thread;
    std::mutex sync;
    std::queue<std::string> queue;
    std::queue<std::string> finished_queue;
public:
    bool init();
    void poll();
    void cleanup();

    void push(const std::string& res_path);

    void _threadProc();
};

#endif
