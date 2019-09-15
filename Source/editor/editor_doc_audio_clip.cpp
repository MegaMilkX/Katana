#include "editor_doc_audio_clip.hpp"

#include "editor.hpp"

#include "../common/audio.hpp"

EditorDocAudioClip::EditorDocAudioClip() {
    chan = audio().createChannel();
    audio().setLooping(chan, true);
}

void EditorDocAudioClip::onResourceSet() {
    auto& clip = _resource;
    audio().setBuffer(chan, clip->getBuffer());
}

void EditorDocAudioClip::onGui(Editor* ed, float dt) {
    auto& clip = _resource;

    ImVec2 winMin = ImGui::GetWindowContentRegionMin();
    ImVec2 winMax = ImGui::GetWindowContentRegionMax();
    ImVec2 winSize = ImVec2(winMax - winMin);

    AudioBuffer* buf = clip->getBuffer();
    for(int i = 0; i < buf->channelCount(); ++i) {
        ImGui::PlotLines(
            MKSTR("Channel " << i).c_str(), 
            [](void* data, int idx)->float{
                short* d = (short*)data;
                return d[idx] / 32767.0f;
            }, buf->getPtr(), buf->sampleCount(), 4410, "test", -1.0f, 1.0f, ImVec2(0, (winSize.y - 50) / buf->channelCount())
        );
    }
    
    if(!audio().isPlaying(chan)) {
        if(ImGui::Button(ICON_MDI_PLAY)) {
            audio().stop(chan);
            audio().play(chan);
        }
    } else {
        if(ImGui::Button(ICON_MDI_PAUSE)) {
            audio().stop(chan);
        }
    }
    ImGui::SameLine();
    if(ImGui::Button(ICON_MDI_STOP)) {
        audio().stop(chan);
        audio().resetCursor(chan);
    } 
}