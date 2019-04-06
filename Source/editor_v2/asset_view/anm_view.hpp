#ifndef ANM_VIEW_HPP
#define ANM_VIEW_HPP

#include "asset_view.hpp"

#include "../common/resource/animation.hpp"

class AnimAssetView : public AssetView {
public:
    AnimAssetView(std::shared_ptr<Animation> anim)
    : anim(anim) {
    }

    virtual void onGui() {
        if(!anim) return;
        ImGui::Text("Animation");
        ImGui::Text(MKSTR(anim->nodeCount() << " nodes animated").c_str());

        struct evt_t {
            float time;
            std::string name;
        };
        std::vector<evt_t> markers;
        for(size_t i = 0; i < anim->eventCount(); ++i) {
            markers.emplace_back(evt_t{anim->getEvent(i).time, anim->getEventName(i)});
        }
        if(ImGui::BeginTimeline("Events", anim->length)) {
            static int selected_id = 0;
            for(size_t i = 0; i < markers.size(); ++i) {
                bool selected = selected_id == i;
                if(ImGui::TimelineMarker(markers[i].name.c_str(), markers[i].time, &selected)) {
                    selected_id = i;
                }                
            }
            
            ImGui::EndTimeline();

            if(anim->eventCount()) {
                float thresh = anim->getEvent(selected_id).threshold;
                float t = anim->getEvent(selected_id).time;
                if(ImGui::DragFloat("threshold", &thresh, 0.01f, 0.0f, 1.0f)) {
                    anim->setEvent(anim->getEventName(selected_id), t, thresh);
                }
                if(ImGui::SmallButton("- remove selected")) {
                    anim->removeEvent(anim->getEventName(selected_id));
                    markers.erase(markers.begin() + selected_id);
                }
            }
            
            static std::string new_evt_name = "new_event";
            char buf[256];
            memset(buf, 0, sizeof(buf));
            memcpy(buf, new_evt_name.c_str(), new_evt_name.size());
            if(ImGui::InputText("New event", buf, sizeof(buf))) {
                new_evt_name = buf;
            }
            if(ImGui::SmallButton("+ add event")) {
                anim->setEvent(new_evt_name, 0.0f);
            }
        }
        for(size_t i = 0; i < markers.size(); ++i) {
            anim->setEvent(markers[i].name, markers[i].time, anim->getEvent(i).threshold);
        }

        for(size_t i = 0; i < anim->curveCount(); ++i) {
            ImGui::Text(anim->getCurveName(i).c_str());

            auto& curve = anim->getCurve(i);
            auto& kfs = curve.get_keyframes();
            std::vector<ImVec2> points(kfs.size() + 1);
            points.resize(kfs.size() + 1);
            for(size_t j = 0; j < kfs.size(); ++j) {
                points[j].x = kfs[j].time;
                points[j].y = kfs[j].value;
            }
            size_t kf_count = points.size() - 1;
            if(ImGui::CurveEdit(MKSTR(i).c_str(), points.data(), kf_count, points.size(), anim->length)) {
                kfs.resize(kf_count);
                for(size_t j = 0; j < kf_count; ++j) {
                    kfs[j].time = points[j].x;
                    kfs[j].value = points[j].y;
                }
            }
            if(ImGui::SmallButton("Remove")) {
                anim->removeCurve(anim->getCurveName(i));
            }
        }

        char buf[256];
        memset(buf, 0, 256);
        memcpy(buf, new_curve_name.data(), new_curve_name.size());
        if(ImGui::InputText("New curve name", buf, 256)) {
            new_curve_name = buf;
        }
        if(ImGui::Button("Add curve")) {
            int i = 0;
            bool already_exists;
            do {
                std::string name = new_curve_name;
                if(i > 0) {
                    name = MKSTR(new_curve_name << i);
                }
                already_exists = anim->curveExists(name);
                anim->getCurve(name);
                ++i;
            } while(already_exists);
        }
    }
private:
    std::shared_ptr<Animation> anim;
    std::string new_curve_name = "curve";
};

#endif
