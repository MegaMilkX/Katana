#include "editor_dir_view.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "../common/resource/texture2d.h"

#include "../common/data_headers/file.png.h"
#include "../common/data_headers/jpg.png.h"
#include "../common/data_headers/json_file.png.h"
#include "../common/data_headers/png.png.h"
#include "../common/data_headers/txt.png.h"
#include "../common/data_headers/file16.png.h"
#include "../common/data_headers/jpg16.png.h"
#include "../common/data_headers/json_file16.png.h"
#include "../common/data_headers/png16.png.h"
#include "../common/data_headers/txt16.png.h"

#include "../common/util/init_filesystem_resources.hpp"

#include "../common/util/has_suffix.hpp"

#include "editor.hpp"

#include <cctype>

void EditorDirView::initDirWatch(const std::string& dir) {
    dwChangeHandle = FindFirstChangeNotificationA(
        dir.c_str(),
        TRUE,
        FILE_NOTIFY_CHANGE_CREATION |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_FILE_NAME
    );
    DWORD err = GetLastError();
    if(dwChangeHandle == INVALID_HANDLE_VALUE) {
        LOG_ERR("FindFirstChangeNotificationA failed: " << dwChangeHandle);
        LOG_ERR("GetLastError: " << err);
        return;
    }
}

void EditorDirView::checkDirChanges() {
    DWORD dwWaitStatus = WaitForSingleObject(dwChangeHandle, 0);
    switch(dwWaitStatus) {
    case WAIT_OBJECT_0:
        makeDirTree(directory, root_dir);
        updateFileList(selected_dir.full_path);
        initFilesystemResources();

        if(FindNextChangeNotification(dwChangeHandle) == FALSE) {
            LOG_ERR("FindNextChangeNotification failed");
            return;
        }
        break;
    }
}

void EditorDirView::init(const std::string& dir) {
    root_dir = DirInfo{
        dir, dir
    };
    directory = dir;

    initDirWatch(dir);
    makeDirTree(directory, root_dir);
    //updateFileList();
}

void EditorDirView::update(Editor* editor) {
    checkDirChanges();
    if(ImGui::Begin("DirView")) {
        if(ImGui::BeginChild("Directories", ImVec2(250, 0))) {
            imguiDirTree(root_dir);
            ImGui::EndChild();
        }
        ImGui::SameLine();
        if(ImGui::BeginChild("Files")) {
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            ImGui::BeginColumns("FileTable", (int)(window_visible_x2 / 400.0f), ImGuiColumnsFlags_NoBorder);
            for(auto& f : filenames) {
                std::string res_name = f.full_path;
                if(res_name.compare(0, get_module_dir().length(), get_module_dir()) == 0) {
                    res_name = res_name.substr(root_dir.full_path.size() + 1);
                }
                while(res_name[0] == '\\' && res_name.size() > 0) {
                    res_name.erase(res_name.begin());
                }

                if(ImGui::Selectable(f.name.c_str(), selected_filename == f.name)) {
                    selected_filename = f.name;
                    editor->getAssetInspector()->setFile(res_name);
                }
                if(ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("DND_ASSET_FILE", res_name.data(), res_name.size() + 1 /* need to capture 0 char too */);
                    //ImGui::Image(icon_tex, button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                    //ImGui::SameLine();
                    ImGui::Text(res_name.c_str());
                    ImGui::EndDragDropSource();
                }
                ImGui::NextColumn();
            }
            ImGui::EndColumns();
            ImGui::EndChild();
        }

        ImGui::End();
    }
}

void EditorDirView::makeDirTree(const std::string& dir_path, DirInfo& dir) {
    dir.children.clear();

    WIN32_FIND_DATAA fd;
    HANDLE hFind = NULL;

    char sPath[2048];
    sprintf_s(sPath, (dir_path + "\\*").c_str());

    if((hFind = FindFirstFileA(sPath, &fd)) == INVALID_HANDLE_VALUE) {
        LOG_ERR("Invalid search path: " << sPath);
        return;
    }
    do {
        std::string full_path = MKSTR(dir_path << "\\" << fd.cFileName);
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if(std::string(fd.cFileName) == "." ||
                std::string(fd.cFileName) == "..") {
                continue;
            }
            dir.children.emplace_back(DirInfo{ fd.cFileName, full_path });
            makeDirTree(full_path, dir.children.back());
        }
    } while(FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

void EditorDirView::updateFileList(const std::string& dir) {
    filenames.clear();

    WIN32_FIND_DATAA fd;
    HANDLE hFind = NULL;

    char sPath[2048];
    sprintf_s(sPath, (dir + "\\*").c_str());

    if((hFind = FindFirstFileA(sPath, &fd)) == INVALID_HANDLE_VALUE) {
        LOG_ERR("Invalid search path: " << sPath);
        return;
    }

    do {
        std::string full_path = MKSTR(dir << "\\" << fd.cFileName);
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        } else {
            // File
            std::string fname = fd.cFileName;
            for(size_t i = 0; i < fname.size(); ++i) {
                fname[0] = std::tolower(fname[0]);
            }
            if(has_suffix(fname, ".asset_params")) {
                continue;
            }
            filenames.emplace_back(FileInfo{ fd.cFileName, full_path });
        }
    } while(FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

void EditorDirView::imguiDirTree(DirInfo& dir) {
    if(dir.children.size() == 0) {
        ImGui::PushID(MKSTR(dir.name << "##" << &dir).c_str());
        if(ImGui::Selectable(MKSTR(dir.name).c_str(), selected_dir.name == dir.name ? true : false)) {
            selected_dir = dir;
            updateFileList(dir.full_path);
        }
        ImGui::PopID();
    } else {
        ImGui::PushID(&dir);
        bool node_open = ImGui::TreeNodeEx(
            (void*)&dir,
            selected_dir.name == dir.name ? ImGuiTreeNodeFlags_Selected : 0 |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_OpenOnArrow,
            MKSTR(dir.name).c_str()
        );
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            selected_dir.name = dir.name;
            updateFileList(dir.full_path);
        }
        ImGui::PopID();
        if(node_open) {
            for(unsigned i = 0; i < dir.children.size(); ++i) {
                imguiDirTree(dir.children[i]);
            }
            ImGui::TreePop();
        }  
    }
}