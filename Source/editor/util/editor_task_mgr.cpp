#include "editor_task_mgr.hpp"

#include <condition_variable>

#include "../common/platform/platform.hpp"

static std::mutex              sync;
static std::condition_variable cv;
static bool working = false;
static std::thread worker;
static std::mutex              task_sync;
static std::vector<edTaskAsync*> tasks;

static void threadFnWorker() {
    platformMakeAsyncRenderContextCurrent();

    std::unique_lock<std::mutex> lock(sync);
    while(working) {
        edTaskAsync* task;
        bool has_tasks = false;
        {
            std::lock_guard<std::mutex> lock(task_sync);
            has_tasks = !tasks.empty();
            if(has_tasks) {
                task = tasks.back();
            }
        }
        if(!has_tasks) {
            cv.wait(lock);
        } else {
            task->execute();
            task->_markDone();
            {
                std::lock_guard<std::mutex> lock(task_sync);
                tasks.pop_back();
            }
        }
    }
}


void edTaskMgrStart() {
    working = true;
    worker = std::thread(&threadFnWorker);
}
void edTaskMgrStop() {
    working = false;
    {
        std::lock_guard<std::mutex> lock(task_sync);
        tasks.clear();
    }
    cv.notify_one();
    worker.join();
}

int edTaskMgrGetTaskCount() {
    std::lock_guard<std::mutex> lock(task_sync);
    return tasks.size();
}

void edTaskMgrGetTaskNameList(std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lock(task_sync);
    for(int i = 0; i < tasks.size(); ++i) {
        names.push_back(tasks[i]->getName());
    }
}

void edTaskPost(edTaskAsync* task) {
    {
        std::lock_guard<std::mutex> lock(task_sync);
        tasks.insert(tasks.begin(), task);
    }
    cv.notify_one();
}