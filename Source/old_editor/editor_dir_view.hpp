#ifndef EDITOR_DIR_VIEW_HPP
#define EDITOR_DIR_VIEW_HPP

#include "editor_window.hpp"

#include "../common_old/util/log.hpp"

#include <stdlib.h>

#include "../common_old/resource/texture2d.h"

#include "../common_old/data_headers/file.png.h"
#include "../common_old/data_headers/jpg.png.h"
#include "../common_old/data_headers/json_file.png.h"
#include "../common_old/data_headers/png.png.h"
#include "../common_old/data_headers/txt.png.h"
#include "../common_old/data_headers/file16.png.h"
#include "../common_old/data_headers/jpg16.png.h"
#include "../common_old/data_headers/json_file16.png.h"
#include "../common_old/data_headers/png16.png.h"
#include "../common_old/data_headers/txt16.png.h"

#include "../common_old/util/init_filesystem_resources.hpp"

#include <cctype>

enum FILE_ICON_TYPE {
    FILE_ICON_DEFAULT,
    FILE_ICON_JPG,
    FILE_ICON_JSON,
    FILE_ICON_PNG,
    FILE_ICON_TXT
};

inline FILE_ICON_TYPE getFileIconId(const std::string& ext) {
    FILE_ICON_TYPE t = FILE_ICON_DEFAULT;
    if(has_suffix(ext, ".jpg") || has_suffix(ext, ".jpeg")) {
        t = FILE_ICON_JPG;
    } else if(has_suffix(ext, ".json") || has_suffix(ext, ".mat")) {
        t = FILE_ICON_JSON;
    } else if(has_suffix(ext, ".png")) {
        t = FILE_ICON_PNG;
    } else if(has_suffix(ext, ".txt")) { 
        t = FILE_ICON_TXT;
    }

    return t;
}

class EditorDirView : public EditorWindow {
public:
    struct FileInfo {
        std::string name;
        std::string full_path;
    };
    struct DirInfo {
        std::string name;
        std::string full_path;
        std::vector<DirInfo> children;
    };

    std::string Name() {
        return "DirView";
    }

    void init(const std::string& dir) {
        root_dir = DirInfo {
            dir, dir
        };
        selected_dir = root_dir;

        directory = dir;
        initDirWatch(dir);
        makeDirTree(dir, root_dir);
        updateList(dir);

        auto make_icon_texture = [](const unsigned char* data, size_t sz, Texture2D& tex) {
            std::shared_ptr<DataSourceMemory> ds_mem(new DataSourceMemory((char*)data, (sz)));
            if(!tex.deserialize(*ds_mem->make_stream().get(), sz)) {
                LOG_ERR("Failed to build file icon texture");
            }
        };

        make_icon_texture(file_png, sizeof(file_png), file_icons64[0]);
        make_icon_texture(jpg_png, sizeof(jpg_png), file_icons64[1]);
        make_icon_texture(json_file_png, sizeof(json_file_png), file_icons64[2]);
        make_icon_texture(png_png, sizeof(png_png), file_icons64[3]);
        make_icon_texture(txt_png, sizeof(txt_png), file_icons64[4]);

        make_icon_texture(file16_png, sizeof(file16_png), file_icons16[0]);
        make_icon_texture(jpg16_png, sizeof(jpg16_png), file_icons16[1]);
        make_icon_texture(json_file16_png, sizeof(json_file16_png), file_icons16[2]);
        make_icon_texture(png16_png, sizeof(png16_png), file_icons16[3]);
        make_icon_texture(txt16_png, sizeof(txt16_png), file_icons16[4]);
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
            root_dir.children.clear();
            makeDirTree(directory, root_dir);
            updateList(selected_dir.full_path + "\\");
            initFilesystemResources();

            if(FindNextChangeNotification(dwChangeHandle) == FALSE) {
                LOG_ERR("FindNextChangeNotification failed");
                return;
            }
            break;
        }
    }

    void updateList(const std::string& dir) {
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
    void makeDirTree(const std::string& dir_path, DirInfo& dir) {
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
            std::string full_path = MKSTR(dir_path << fd.cFileName);
            if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if(std::string(fd.cFileName) == "." ||
                    std::string(fd.cFileName) == "..") {
                    continue;
                }
                dir.children.emplace_back(DirInfo{ fd.cFileName, full_path });
                makeDirTree(full_path + "\\", dir.children.back());
            }
        } while(FindNextFileA(hFind, &fd));

        FindClose(hFind);
    }

    void imguiDirTree(DirInfo& dir) {
        if(dir.children.size() == 0) {
            ImGui::PushID(MKSTR(dir.name << "##" << &dir).c_str());
            if(ImGui::Selectable(MKSTR(dir.name).c_str(), selected_dir.name == dir.name ? true : false)) {
                selected_dir = dir;
                updateList(dir.full_path);
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
                updateList(dir.full_path);
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

    void Update() {
        checkDirChanges();

        ImGui::BeginChild("FileViewControls", ImVec2(0, 30));
        ImGui::RadioButton("Small", &view_mode, 0);
        ImGui::SameLine(); 
        ImGui::RadioButton("Large", &view_mode, 1);
        ImGui::EndChild();

        if(ImGui::BeginChild("Directories", ImVec2(250, 0))) {
            imguiDirTree(root_dir);
            ImGui::EndChild();
        } 
        ImGui::SameLine();

        if(ImGui::BeginChild("Files")) {
            ImGuiStyle& style = ImGui::GetStyle();
            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            size_t buttons_count = filenames.size();

            if(view_mode == 0) {
                ImVec2 button_sz(16, 16);
                ImGui::BeginColumns("FileTable", (int)(window_visible_x2 / 400.0f), ImGuiColumnsFlags_NoBorder);
                int i = 0;
                for(auto& f : filenames) {
                    ImVec4 img_tint = ImVec4(1.0f,1.0f,1.0f,1);

                    ImGui::BeginChild(i, ImVec2(0, 20));
                    
                    std::string res_name = f.full_path;
                    if(res_name.compare(0, get_module_dir().length(), get_module_dir()) == 0) {
                        res_name = res_name.substr(get_module_dir().length() + 1);
                    }
                    while(res_name[0] == '\\' && res_name.size() > 0) {
                        res_name.erase(res_name.begin());
                    }
                    std::string ext_check_string;
                    ext_check_string.resize(res_name.size());
                    for(size_t i = 0; i < res_name.size(); ++i) {
                        ext_check_string[i] = (std::tolower(res_name[i]));
                    }

                    ImTextureID icon_tex = (ImTextureID)file_icons16[getFileIconId(ext_check_string)].GetGlName();
                    
                    if(ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("DND_ASSET_FILE", res_name.data(), res_name.size() + 1 /* need to capture 0 char too */);
                        ImGui::Image(icon_tex, button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                        ImGui::SameLine();
                        ImGui::Text(f.name.c_str());
                        ImGui::EndDragDropSource();
                    }
                    
                    std::string fname = f.name;
                    if(fname.size() > 32) {
                        fname = fname.substr(0, 32);
                        fname = fname + "...";
                    }
                    ImGui::Image(icon_tex, button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                    ImGui::SameLine();
                    ImGui::TextWrapped(fname.c_str());
                    ImGui::EndChild();
                
                    ImGui::NextColumn();
                    ++i;
                }
                ImGui::EndColumns();
            } else {
                ImVec2 button_sz(64, 64);
                ImGui::BeginColumns("FileTable", (int)(window_visible_x2 / 150.0f), ImGuiColumnsFlags_NoBorder);
                int i = 0;
                for(auto& f : filenames) {
                    ImVec4 img_tint = ImVec4(1.0f,1.0f,1.0f,1);

                    ImGui::BeginChild(i, ImVec2(0, 120));
                    
                    std::string res_name = f.full_path;
                    if(res_name.compare(0, get_module_dir().length(), get_module_dir()) == 0) {
                        res_name = res_name.substr(get_module_dir().length() + 1);
                    }
                    while(res_name[0] == '\\' && res_name.size() > 0) {
                        res_name.erase(res_name.begin());
                    }
                    std::string ext_check_string;
                    ext_check_string.resize(res_name.size());
                    for(size_t i = 0; i < res_name.size(); ++i) {
                        ext_check_string[i] = (std::tolower(res_name[i]));
                    }

                    ImTextureID icon_tex = (ImTextureID)file_icons64[getFileIconId(ext_check_string)].GetGlName();
                    
                    if(ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("DND_ASSET_FILE", res_name.data(), res_name.size() + 1 /* need to capture 0 char too */);
                        ImGui::Image(icon_tex, button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                        ImGui::Text(f.name.c_str());
                        ImGui::EndDragDropSource();
                    }
                    
                    std::string fname = f.name;
                    if(fname.size() > 16) {
                        fname = fname.substr(0, 16);
                        fname = fname + "...";
                    }
                    ImGui::Image(icon_tex, button_sz, ImVec2(0, 1), ImVec2(1, 0), img_tint);
                    ImGui::TextWrapped(fname.c_str());
                    ImGui::EndChild();
                
                    ImGui::NextColumn();
                    ++i;
                }
                ImGui::EndColumns();
            }
            
            ImGui::EndChild();
        }
    }
private:
    int view_mode = 0;
    DirInfo selected_dir;

    Texture2D file_icons64[5];
    Texture2D file_icons16[5];

    std::string directory;
    DirInfo root_dir;
    std::vector<FileInfo> filenames;
    HANDLE dwChangeHandle;
};

#endif
