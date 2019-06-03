#ifndef EDITOR_ASSET_INSPECTOR_HPP
#define EDITOR_ASSET_INSPECTOR_HPP

#include <string>
#include "../common/resource/resource.h"

class Editor;
class AssetView;
class EditorAssetInspector {
public:
    EditorAssetInspector();
    void setFile(const std::string& fname);
    void update(Editor* editor);
private:
    std::string file_name;
    std::shared_ptr<AssetView> asset_view;
    std::shared_ptr<Resource> loaded_resource;
};

#endif
