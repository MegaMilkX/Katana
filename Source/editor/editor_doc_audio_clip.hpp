#ifndef EDITOR_DOC_AUDIO_CLIP_HPP
#define EDITOR_DOC_AUDIO_CLIP_HPP

#include "editor_document.hpp"
#include "../common/resource/audio_clip.hpp"
#include "../common/resource/resource_tree.hpp"

class EditorDocAudioClip : public EditorDocument {
    std::shared_ptr<AudioClip> clip;
    size_t chan = 0;
public:
    EditorDocAudioClip(ResourceNode* node);

    virtual void onGui(Editor* ed);
};

#endif
