#include "thumb_builder.hpp"

#include <windows.h>
#include "../common/util/filesystem.hpp"
#include "../common/util/log.hpp"

#include "preview_library.hpp"

static void thumbBuilderThreadProc() {
    ThumbBuilder::get()->_threadProc();
}

bool ThumbBuilder::init() {
    working = true;
    thread = std::thread(&thumbBuilderThreadProc);
    return true;
}
void ThumbBuilder::cleanup() {
    working = false;
    thread.join();
}

void ThumbBuilder::poll() {
    std::string path;
    {
        std::lock_guard<std::mutex> lock(sync);
        if(!finished_queue.empty()) {
            path = finished_queue.front();
            finished_queue.pop();
        }
    }
    if(path.empty()) {
        return;
    }

    PreviewLibrary::get()->markForReload(path);
}

void ThumbBuilder::push(const std::string& res_path) {
    std::lock_guard<std::mutex> lock(sync);
    queue.push(res_path);
}

void ThumbBuilder::clear_queue() {
    std::lock_guard<std::mutex> lock(sync);
    while(!queue.empty()) {
        queue.pop();
    }
}

void ThumbBuilder::_threadProc() {
    while(working) {
        std::string path;
        sync.lock();
        if(!queue.empty()) {
            path = queue.front();
            queue.pop();
            sync.unlock();
        } else {
            sync.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const std::string cmd_line = 
            MKSTR(get_module_dir() << "/thumb_builder.exe \"" << path << "\""); 

        STARTUPINFO info = { sizeof(info) };
        PROCESS_INFORMATION proc_info;
        ZeroMemory(&proc_info, sizeof(proc_info));
        BOOL ret = CreateProcessA(
            0, 
            (char*)cmd_line.c_str(),
            NULL, NULL, TRUE, 0, NULL, 
            get_module_dir().c_str(), 
            &info,
            &proc_info
        );
        if(ret == FALSE) {
            LOG_WARN("Failed to launch thumb builder process");
            return;
        }
        
        WaitForSingleObject(proc_info.hProcess, INFINITE);
        DWORD exit_code = 0;
        ret = GetExitCodeProcess(proc_info.hProcess, &exit_code);
        if(!ret) {
            LOG_WARN("Failed to get thumb builder exit code");
            return;
        }
        CloseHandle(proc_info.hProcess);
        CloseHandle(proc_info.hThread);

        if(exit_code == 0) {
            std::lock_guard<std::mutex> lock(sync);
            finished_queue.push(path);
        } else {
            std::lock_guard<std::mutex> lock(sync);
            finished_queue.push(path);
        }
    }
}