#ifndef EDITOR_DIR_VIEW_HPP
#define EDITOR_DIR_VIEW_HPP

#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

class EditorDirView {
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

    void init(const std::string& dir);
    void update();
private:
    void initDirWatch(const std::string& dir);
    void checkDirChanges();

    void makeDirTree(const std::string& dir_path, DirInfo& dir);
    void updateFileList(const std::string& dir);

    void imguiDirTree(DirInfo& dir);

    DirInfo selected_dir;
    std::string directory;
    DirInfo root_dir;
    std::vector<FileInfo> filenames;
    HANDLE dwChangeHandle;
};

#endif
