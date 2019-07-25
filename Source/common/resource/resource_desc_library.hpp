#ifndef RESOURCE_DESC_LIBRARY_HPP
#define RESOURCE_DESC_LIBRARY_HPP

#include <map>
#include <string>

#include "resource_desc.hpp"

#include "../util/singleton.hpp"

#include "texture2d.h"

#include "../../editor/editor_doc_scene.hpp"
#include "../../editor/editor_doc_texture2d.hpp"
#include "../../editor/editor_doc_audio_clip.hpp"
#include "../../editor/editor_doc_model_source.hpp"

class ResourceDescLibrary : public Singleton<ResourceDescLibrary> {
    std::map<std::string, ResourceDesc> descs;
public:

    void init() {
        descs = {
            { "so", ResourceDesc{
                rttr::type::get<GameScene>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocScene(node);
                }
            }},
            { "png", ResourceDesc{
                rttr::type::get<Texture2D>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocTexture2d(node);
                }
            }},
            { "jpg", ResourceDesc{
                rttr::type::get<Texture2D>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocTexture2d(node);
                }
            }},
            { "jpeg", ResourceDesc{
                rttr::type::get<Texture2D>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocTexture2d(node);
                }
            }},
            { "jfif", ResourceDesc{
                rttr::type::get<Texture2D>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocTexture2d(node);
                }
            }},
            { "tga", ResourceDesc{
                rttr::type::get<Texture2D>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocTexture2d(node);
                }
            }},
            { "ogg", ResourceDesc{
                rttr::type::get<AudioClip>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocAudioClip(node);
                }
            }},
            { "fbx", ResourceDesc{
                rttr::type::get<ModelSource>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocModelSource(node);
                }
            }},
            { "obj", ResourceDesc{
                rttr::type::get<ModelSource>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocModelSource(node);
                }
            }},
            { "dae", ResourceDesc{
                rttr::type::get<ModelSource>(),
                [](ResourceNode* node)->EditorDocument*{
                    return new EditorDocModelSource(node);
                }
            }}
        };
    }

    ResourceDesc* find(const std::string& ext) {
        auto it = descs.find(ext);
        if(it != descs.end()) {
            return &it->second;
        } else {
            return 0;
        }
    }
};

#endif
