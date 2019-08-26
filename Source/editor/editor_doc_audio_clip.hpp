#ifndef EDITOR_DOC_AUDIO_CLIP_HPP
#define EDITOR_DOC_AUDIO_CLIP_HPP

#include "editor_document.hpp"
#include "../common/resource/audio_clip.hpp"
#include "../common/resource/resource_tree.hpp"

class EditorDocAudioClip : public EditorDocumentTyped<AudioClip> {
    size_t chan = 0;
public:
    EditorDocAudioClip(std::shared_ptr<ResourceNode>& node);

    virtual void setResourceNode(std::shared_ptr<ResourceNode>& node);

    virtual void onGui(Editor* ed, float dt);
};

#endif
