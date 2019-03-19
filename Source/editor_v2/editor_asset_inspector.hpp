#ifndef EDITOR_ASSET_INSPECTOR_HPP
#define EDITOR_ASSET_INSPECTOR_HPP

#include <string>

class Editor;
class AssetView;
class EditorAssetInspector {
public:
    void setFile(const std::string& fname);
    void update(Editor* editor);
private:
    std::string file_name;
    std::shared_ptr<AssetView> asset_view;
};

#endif
