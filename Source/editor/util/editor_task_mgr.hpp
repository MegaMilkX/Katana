#ifndef EDITOR_TASK_MGR_HPP
#define EDITOR_TASK_MGR_HPP

#include <thread>
#include <string>
#include "../common/util/log.hpp"

class edTaskAsync {
    std::string name;
    std::mutex  done_sync;
    bool        is_done = false;

public:
    edTaskAsync(const char* name = 0)
    : name(name) {

    }
    virtual ~edTaskAsync() {}

    const char* getName() const {
        return name.c_str();
    }

    void reportProgress(const char* message, float progress) {
        LOG_WARN(message << ", " << progress * 100);
    }

    virtual void execute() = 0;

    bool isDone() {
        std::lock_guard<std::mutex> lock(done_sync);
        return is_done;
    }

    void _markDone() {
        std::lock_guard<std::mutex> lock(done_sync);
        is_done = true;
    }
};

void edTaskMgrStart();
void edTaskMgrStop();

int  edTaskMgrGetTaskCount();
void edTaskMgrGetTaskNameList(std::vector<std::string>& names);

void edTaskPost(edTaskAsync* task);


#endif
