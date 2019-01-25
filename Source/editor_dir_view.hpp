#ifndef EDITOR_DIR_VIEW_HPP
#define EDITOR_DIR_VIEW_HPP

#include "editor_window.hpp"

#include "util/log.hpp"

#include <stdlib.h>

#include "resource/texture2d.h"

#include "data_headers/file_icon.png.h"

#include "util/init_filesystem_resources.hpp"

class EditorDirView : public EditorWindow {
public:
    struct FileInfo {
        std::string name;
        std::string full_path;
    };

    std::string Name() {
        return "DirView";
    }

    void init(const std::string& dir) {
        directory = dir;
        initDirWatch(dir);
        updateList(dir);

        std::shared_ptr<DataSourceMemory> ds_mem(new DataSourceMemory((char*)file_icon_png, sizeof(file_icon_png)));
        if(!file_icon.Build(ds_mem)) {
            LOG_ERR("Failed to build file icon texture");
        }
    }

    void initDirWatch(const std::string& dir) {
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

    void checkDirChanges() {
        DWORD dwWaitStatus = WaitForSingleObject(dwChangeHandle, 0);
        switch(dwWaitStatus) {
        case WAIT_OBJECT_0:
            updateList(directory);
            initFilesystemResources(get_module_dir());

            if(FindNextChangeNotification(dwChangeHandle) == FALSE) {
                LOG_ERR("FindNextChangeNotification failed");
                return;
            }
            break;
        }
    }

    void updateList(const std::string& dir) {
        filenames.clear();
        dirs.clear();

        WIN32_FIND_DATAA fd;
        HANDLE hFind = NULL;

        char sPath[2048];
        sprintf_s(sPath, (dir + "\\*").c_str());

        if((hFind = FindFirstFileA(sPath, &fd)) == INVALID_HANDLE_VALUE) {
            LOG_ERR("Invalid search path: " << sPath);
            return;
        }

        do {
            std::string full_path = MKSTR(dir << fd.cFileName);
            if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Directory
                dirs.emplace_back(FileInfo{ fd.cFileName, full_path });
            } else {
                // File
                filenames.emplace_back(FileInfo{ fd.cFileName, full_path });
            }
        } while(FindNextFileA(hFind, &fd));

        FindClose(hFind);
    }
    void Update() {
        checkDirChanges();
        if(ImGui::BeginChild("Directories", ImVec2(200, 0))) {
            for(auto& d : dirs) {
                ImGui::Text(d.name.c_str());
            }
            ImGui::EndChild();
        } 
        ImGui::SameLine();
        if(ImGui::BeginChild("Files")) {
            ImGuiStyle& style = ImGui::GetStyle();
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            ImVec2 button_sz(32, 32);
            size_t buttons_count = filenames.size();
            
            ImGui::BeginColumns("FileTable", (int)(window_visible_x2 / 150.0f), ImGuiColumnsFlags_NoBorder);
            int i = 0;
            for(auto& f : filenames) {
                ImVec4 img_tint = ImVec4(0.7f,0.7f,0.7f,1);
                /*
                if(ImGui::IsItemHovered()) {
                    img_tint = ImVec4(1,1,1,1);
                }*/

                //ImGui::BeginGroup();
                ImGui::BeginChild(i, ImVec2(0, 70));
                
                std::string res_name = f.full_path;
                if(res_name.compare(0, get_module_dir().length(), get_module_dir()) == 0) {
                    res_name = res_name.substr(get_module_dir().length() + 1);
                }
                
                if(ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("DND_ASSET_FILE", res_name.data(), res_name.size() + 1 /* need to capture 0 char too */);
                    ImGui::Image((ImTextureID)file_icon.GetGlName(), button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                    ImGui::Text(f.name.c_str());
                    ImGui::EndDragDropSource();
                }
                
                ImGui::Image((ImTextureID)file_icon.GetGlName(), button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                ImGui::TextWrapped(f.name.c_str());
                ImGui::EndChild();
                //ImGui::EndGroup();
            
                ImGui::NextColumn();
                ++i;
            }
            ImGui::EndColumns();

            
            ImGui::EndChild();
        }
    }
private:
    Texture2D file_icon;
    std::string directory;
    std::vector<FileInfo> dirs;
    std::vector<FileInfo> filenames;
    HANDLE dwChangeHandle;
};

#endif
