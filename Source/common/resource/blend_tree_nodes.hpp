#ifndef BLEND_TREE_NODES_HPP
#define BLEND_TREE_NODES_HPP

#include "../util/func_graph/node_graph.hpp"
#include "animation.hpp"
#include "skeleton.hpp"

#include "../util/imgui_helpers.hpp"
#include "../util/imgui_ext.hpp"

#include "blend_tree.hpp"
#include "motion.hpp"

#include "../lib/delaunator-cpp/delaunator.hpp"


class BlendTree;

inline float triangle_area(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& c) {
    return abs((a.x*(b.y-c.y) + b.x*(c.y-a.y)+ c.x*(a.y-b.y))/2.0f); 
}
inline float point_inside_triangle(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& c, const gfxm::vec2& pt) {
    float A  = triangle_area(a, b, c);
    float A1 = triangle_area(pt, b, c);
    float A2 = triangle_area(pt, a, c);
    float A3 = triangle_area(pt, a, b);
    return abs(A - (A1 + A2 + A3)) < 0.000001f;
}
inline float distance_to_line(const gfxm::vec2& a, const gfxm::vec2& b, const gfxm::vec2& pt) {
    gfxm::vec2 V  = b - a;
    gfxm::vec2 Vp = pt - a;
    float Lv      = gfxm::length(V);
    float dot     = gfxm::dot(V / Lv, Vp / Lv);
    if (dot > 1.0f) {
        return gfxm::length(pt - b);
    } else if (dot < .0f) {
        return gfxm::length(pt - a);
    } else {
        return gfxm::length(Vp - V * dot);
    }
}

class Blend2dJob : public JobNode<Blend2dJob, BlendTree> {
    Pose pose;
    Pose pa, pb, pc;

    struct Anim {
        std::shared_ptr<Animation> anim         = 0;
        gfxm::vec2                 blend_pos;
        std::vector<int32_t>       mapping;
        float                      influence    = .0f;
        float                      distance     = .0f;
    };
    struct Edge {
        Anim* a;
        Anim* b;
    };
    struct Triangle {
        Anim* a;
        Anim* b;
        Anim* c;
    };
    std::vector<Anim>       animations;
    gfxm::vec2              blend_point;
    gfxm::vec2              snap_steps = gfxm::vec2(10, 10);
    std::vector<Edge>       edges;
    std::vector<Edge>       hull_edges;
    std::vector<Triangle>   triangles;

    void tryInit() {
        auto skel = graph->getMotion()->getSkeleton();
        if(skel) {
            pose.sample_buffer = AnimSampleBuffer(skel.get());
            pa.sample_buffer = AnimSampleBuffer(skel.get());
            pb.sample_buffer = AnimSampleBuffer(skel.get());
            pc.sample_buffer = AnimSampleBuffer(skel.get());
            for(int i = 0; i < animations.size(); ++i) {
                auto& a = animations[i];
                if(!a.anim) {
                    continue;
                }
                a.mapping = a.anim->getMapping(skel.get());
            }
        }
    }

public:
    void onInit(BlendTree* bt) override {
        bind<Pose>(&pose);
        tryInit();
    }
    void onInvoke() override {
        if(is_connected(0)) {
            blend_point.x = get<float>(0);
        }
        if(is_connected(1)) {
            blend_point.y = get<float>(1);
        }
        for(int i = 0; i < animations.size(); ++i) {
            animations[i].influence = .0f;
        }

        edges.clear();
        hull_edges.clear();
        triangles.clear();
        if (animations.size() > 2) {
            gfxm::vec2 N = gfxm::normalize(animations[1].blend_pos - animations[0].blend_pos);
            bool collinear = true;
            for(int i = 2; i < animations.size(); ++i) {
                gfxm::vec2 N2 = gfxm::normalize(animations[i].blend_pos - animations[0].blend_pos);
                float dot = abs(gfxm::dot(N, N2));
                if(1.0f - dot > FLT_EPSILON) {
                    collinear = false;
                    break;
                }
            }
            if(!collinear) {
                std::vector<double> coords;
                for (int i = 0; i < animations.size(); ++i) {
                    coords.push_back(animations[i].blend_pos.x);
                    coords.push_back(animations[i].blend_pos.y);
                }
                delaunator::Delaunator d(coords);
                if(d.is_valid()) {
                    for (int i = 0; i < d.triangles.size(); i += 3) {
                        size_t a = d.triangles[i];
                        size_t b = d.triangles[i + 1];
                        size_t c = d.triangles[i + 2];
                        edges.push_back(Edge{ &animations[a], &animations[b] });
                        edges.push_back(Edge{ &animations[b], &animations[c] });
                        edges.push_back(Edge{ &animations[c], &animations[a] });

                        triangles.push_back(Triangle{ &animations[a], &animations[b], &animations[c] });
                    }

                    size_t e = d.hull_start;
                    do {
                        hull_edges.push_back(Edge{ &animations[d.hull_prev[e]], &animations[e] });
                        e = d.hull_next[e];
                    } while (e != d.hull_start);
                }
            }
        }

        std::vector<Anim*> blend_group;
        Triangle tri = { 0 };
        if(triangles.size() > 0) {
            bool inside = false;
            for(int i = 0; i < triangles.size(); ++i) {
                inside = point_inside_triangle(
                    triangles[i].a->blend_pos,
                    triangles[i].b->blend_pos,
                    triangles[i].c->blend_pos,
                    blend_point
                );
                if(inside) {
                    tri = triangles[i];
                    break;
                }
            }
            if(inside) {
                blend_group.push_back(tri.a);
                blend_group.push_back(tri.b);
                blend_group.push_back(tri.c);
            } else {
                std::vector<Anim*> vec;
                for(int i = 0; i < animations.size(); ++i) {
                    vec.push_back(&animations[i]);
                }
                std::sort(hull_edges.begin(), hull_edges.end(), [this](const Edge& a, const Edge& b){
                    return distance_to_line(a.a->blend_pos, a.b->blend_pos, blend_point) <
                        distance_to_line(b.a->blend_pos, b.b->blend_pos, blend_point);
                });
                blend_group.push_back(hull_edges[0].a);
                blend_group.push_back(hull_edges[0].b);
            }
        } else if (animations.size() > 0 && animations.size() < 3){
            for(int i = 0; i < animations.size(); ++i) {
                blend_group.push_back(&animations[i]);
            }
            if(blend_group.size() == 2) {
                edges.push_back(Edge{ blend_group[0], blend_group[1] });
            }
        } else if (animations.size() > 0) {
            std::vector<Anim*> vec;
            for(int i = 0; i < animations.size(); ++i) {
                vec.push_back(&animations[i]);
            }
            std::sort(vec.begin(), vec.end(), [](const Anim* a, const Anim* b){
                if(a->blend_pos.x == b->blend_pos.x) {
                    return a->blend_pos.y < b->blend_pos.y;
                } else {
                    return a->blend_pos.x < b->blend_pos.x;
                }
            });
            for(int i = 1; i < vec.size(); ++i) {
                edges.push_back(Edge{ vec[i - 1], vec[i] });
            }
            
            std::sort(edges.begin(), edges.end(), [this](const Edge& a, const Edge& b) {
                return distance_to_line(a.a->blend_pos, a.b->blend_pos, blend_point) <
                    distance_to_line(b.a->blend_pos, b.b->blend_pos, blend_point);
            });
            blend_group.push_back(edges[0].a);
            blend_group.push_back(edges[0].b);
        }

        for(int i = 0; i < blend_group.size(); ++i) {
            blend_group[i]->distance = gfxm::length(blend_group[i]->blend_pos - blend_point);
        }
        std::sort(blend_group.begin(), blend_group.end(), [](const Anim* a, const Anim* b) {
            return a->distance < b->distance;
        });

        Skeleton* skel = graph->getMotion()->getSkeleton().get();
        if(blend_group.size() == 1) {
            Anim* a = blend_group[0];
            float cur = graph->getCursor();
            float pcur = graph->getPrevCursor();
            float alen = a->anim->length;
            a->anim->sample_remapped(pose.sample_buffer, cur * alen, pcur * alen, skel, a->mapping);
            a->influence = 1.0f;
        } else if(blend_group.size() == 2) {
            Anim* a = blend_group[0];
            Anim* b = blend_group[1];
            float cur = graph->getCursor();
            float pcur = graph->getPrevCursor();
            float alen = a->anim->length;
            float blen = b->anim->length;
            gfxm::vec2 Vab  = b->blend_pos - a->blend_pos;
            float      Lab  = gfxm::length(Vab);
            gfxm::vec2 Vap  = blend_point - a->blend_pos;
            float      dot  = gfxm::dot(Vab / Lab, Vap / Lab);
            dot             = gfxm::_max(.0f, gfxm::_min(1.0f, dot));
            a->influence = 1.0f - dot;
            b->influence = dot;

            a->anim->sample_remapped(pose.sample_buffer, cur * alen, pcur * alen, skel, a->mapping);
            b->anim->sample_remapped(pb.sample_buffer, cur * blen, pcur * blen, skel, b->mapping);
            for(int i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
                pose.sample_buffer[i].t = gfxm::lerp(pose.sample_buffer[i].t, pb.sample_buffer[i].t, dot);
                pose.sample_buffer[i].r = gfxm::slerp(pose.sample_buffer[i].r, pb.sample_buffer[i].r, dot);
                pose.sample_buffer[i].s = gfxm::lerp(pose.sample_buffer[i].s, pb.sample_buffer[i].s, dot);
            }
        } else if(blend_group.size() == 3) { // Triangle type
            Anim* a = blend_group[0];
            Anim* b = blend_group[1];
            Anim* c = blend_group[2];
            if(a->anim && b->anim && c->anim) {
                float cur = graph->getCursor();
                float pcur = graph->getPrevCursor();
                float alen = a->anim->length;
                float blen = b->anim->length;
                float clen = c->anim->length;

                gfxm::vec2 A = a->blend_pos;
                gfxm::vec2 B = b->blend_pos;
                gfxm::vec2 C = c->blend_pos;
                // Baricentric coordinates
                a->influence = 
                    ((B.y - C.y) * (blend_point.x - C.x) + (C.x - B.x) * (blend_point.y - C.y)) /
                    ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
                b->influence = 
                    ((C.y - A.y) * (blend_point.x - C.x) + (A.x - C.x) * (blend_point.y - C.y)) /
                    ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
                c->influence = 1.0f - a->influence - b->influence;

                a->anim->sample_remapped(pa.sample_buffer, cur * alen, pcur * alen, skel, a->mapping);
                b->anim->sample_remapped(pb.sample_buffer, cur * blen, pcur * blen, skel, b->mapping);
                c->anim->sample_remapped(pc.sample_buffer, cur * clen, pcur * clen, skel, c->mapping);

                for(int i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
                    gfxm::quat qa = pa.sample_buffer[i].r;
                    gfxm::quat qb = pb.sample_buffer[i].r;
                    gfxm::quat qc = pc.sample_buffer[i].r;
                    float abw = a->influence + b->influence;
                    float law = a->influence / abw;
                    float lbw = b->influence / abw;
                    gfxm::quat q = gfxm::slerp(qa, qb, lbw);
                    q = gfxm::slerp(q, qc, c->influence);

                    pose.sample_buffer[i].t = 
                        pa.sample_buffer[i].t * a->influence +
                        pb.sample_buffer[i].t * b->influence +
                        pc.sample_buffer[i].t * c->influence;
                    pose.sample_buffer[i].r = q;
                    pose.sample_buffer[i].s = 
                        pa.sample_buffer[i].s * a->influence +
                        pb.sample_buffer[i].s * b->influence +
                        pc.sample_buffer[i].s * c->influence;
                }
            }
        }
    }

    void onGui() override {
        if(ImGui::DragFloat2("params", (float*)&blend_point, 0.001f, .0f, 1.0f)) {
            if(is_connected(0)) {
                override_input<float>(0, blend_point.x);
            }
            if(is_connected(1)) {
                override_input<float>(1, blend_point.y);
            }
        }
        if(ImGui::DragFloat2("snap step", (float*)&snap_steps, 1.0f, .0f, 100.0f)) {}
        ImGui::PushItemWidth(-1);
        if(ImGuiExt::BeginBlendspace("", ImVec2(0,0), ImVec2(snap_steps.x, snap_steps.y))) {
            for(int i = 0; i < edges.size(); ++i) {
                gfxm::vec2 a_ = edges[i].a->blend_pos;
                gfxm::vec2 b_ = edges[i].b->blend_pos;
                ImVec2 a(a_.x, a_.y);
                ImVec2 b(b_.x, b_.y);
                ImGuiExt::BlendspaceLine(a, b, ImGui::GetColorU32(ImGuiCol_Text));
            }
            for(int i = 0; i < animations.size(); ++i) {
                auto& a = animations[i];
                std::string name = "<null>";
                if(a.anim) {
                    name = a.anim->Name();
                }
                ImVec2 pos(a.blend_pos.x, a.blend_pos.y);
                ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
                if(a.influence > .0f) {
                    col = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
                }
                float size_add = 4 * a.influence;
                if(ImGuiExt::BlendspacePoint(MKSTR(a.influence << " " << name << "###Anim" << i).c_str(), pos, 4 + size_add, ImVec2(snap_steps.x, snap_steps.y), col)) {
                    a.blend_pos.x = pos.x;
                    a.blend_pos.y = pos.y;
                }
            }
            ImU32 col_pt = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
            ImVec2 bp(blend_point.x, blend_point.y);
            if(ImGuiExt::BlendspacePoint("blend", bp, 4, ImVec2(0,0), col_pt)) {
                blend_point.x = bp.x;
                blend_point.y = bp.y;
                blend_point.x = gfxm::_max(.0f, gfxm::_min(1.0f, blend_point.x));
                blend_point.y = gfxm::_max(.0f, gfxm::_min(1.0f, blend_point.y));
                if(is_connected(0)) {
                    override_input<float>(0, blend_point.x);
                }
                if(is_connected(1)) {
                    override_input<float>(1, blend_point.y);
                }
            }
            ImGuiExt::EndBlendspace();
        }
        ImGui::PopItemWidth();
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".anm")) {
                    ImVec2 rmin = ImGui::GetItemRectMin();
                    ImVec2 rmax = ImGui::GetItemRectMax();
                    rmin = rmin + (rmax - rmin) * 0.1f;
                    rmax = rmax - (rmax - rmin) * 0.1f;
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    ImVec2 xy    = (mouse - rmin);
                    ImVec2 rsz   = rmax - rmin;
                    Anim anim;
                    anim.anim = node->getResource<Animation>();
                    gfxm::vec2 pos = gfxm::vec2(xy.x / rsz.x, xy.y / rsz.y);
                    if(snap_steps.x != .0f) {
                        pos.x *= snap_steps.x;
                        pos.x = round(pos.x);
                        pos.x /= snap_steps.x;
                    }
                    if(snap_steps.y != .0f) {
                        pos.y *= snap_steps.y;
                        pos.y = round(pos.y);
                        pos.y /= snap_steps.y;
                    }
                    anim.blend_pos = pos;
                    animations.emplace_back(anim);
                    tryInit();
                }
            }
            ImGui::EndDragDropTarget();
        }

        int delete_index = -1;
        for(int i = 0; i < animations.size(); ++i) {
            if(ImGui::Button(MKSTR(ICON_MDI_CLOSE "###remove" << i).c_str())) {
                delete_index = i;
            } ImGui::SameLine();
            imguiResourceTreeCombo(MKSTR("clip###Clip" << i).c_str(), animations[i].anim, "anm", [this](){
                tryInit();
            });
            if(ImGui::DragFloat2(MKSTR("pos###pos" << i).c_str(), (float*)&animations[i].blend_pos, 0.001f, 0.0f, 1.0f)) {
                
            }
        }
        if(delete_index >= 0) {
            animations.erase(animations.begin() + delete_index);
        }
        if(ImGui::Button("Add clip", ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
            animations.emplace_back(Anim());
        }
    }

    void write(out_stream& out) override {
        DataWriter w(&out);
        w.write<uint32_t>(animations.size());
        for(int i = 0; i < animations.size(); ++i) {
            auto& a = animations[i];
            if(a.anim) {
                w.write(a.anim->Name());
            } else {
                w.write(std::string(""));
            }
            w.write(a.blend_pos);
        }
        w.write(blend_point);
        w.write(snap_steps);
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        uint32_t anim_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < anim_count; ++i) {
            std::string name = r.readStr();
            std::shared_ptr<Animation> anim = getResource<Animation>(name);
            gfxm::vec2 bp = r.read<gfxm::vec2>();
            Anim a;
            a.anim = anim;
            a.blend_pos = bp;
            animations.emplace_back(a);
        }
        blend_point = r.read<gfxm::vec2>();
        snap_steps = r.read<gfxm::vec2>();
    }
};

class BlendAddJob : public JobNode<BlendAddJob, BlendTree> {
    Pose pose;

public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override {
        Pose& target = get<Pose>(0);
        Pose& p = get<Pose>(1);
        Pose& ref = get<Pose>(2);

        auto& p_samples = p.sample_buffer.getSamples();
        auto& ref_samples = ref.sample_buffer.getSamples();
        auto& tgt_samples = target.sample_buffer.getSamples();

        for(int i = 0; i < p_samples.size() && i < ref_samples.size(); ++i) {
            auto& q = p_samples[i].r;
            auto& ref_q = ref_samples[i].r;
            auto& tgt_q = tgt_samples[i].r;
            auto& t = p_samples[i].t;
            auto& ref_t = ref_samples[i].t;
            auto& tgt_t = tgt_samples[i].t;

            pose.sample_buffer[i].r = gfxm::inverse(ref_q) * q * tgt_q;
            pose.sample_buffer[i].t = t - ref_t + tgt_t;
        }
    }

};

class Blend2Job : public JobNode<Blend2Job, BlendTree> {
    Pose pose;
    std::vector<std::string> blend_root_bones;
    std::vector<float> filter_weights;

    void reinit();

public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override {
        float w = get<float>(2);
        w = gfxm::_max(.0f, gfxm::_min(1.0f, w));

        Pose& a = get<Pose>(0);
        Pose& b = get<Pose>(1);
        if (a.sample_buffer.sampleCount() == 0 || b.sample_buffer.sampleCount() == 0) {
            return;
        }

        for(size_t i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
            float fw = filter_weights[i];
            pose.sample_buffer[i].t = gfxm::lerp(a.sample_buffer[i].t, b.sample_buffer[i].t, w * fw);
            pose.sample_buffer[i].r = gfxm::slerp(a.sample_buffer[i].r, b.sample_buffer[i].r, w * fw);
            pose.sample_buffer[i].s = gfxm::lerp(a.sample_buffer[i].s, b.sample_buffer[i].s, w * fw);
        }
        if(a.speed == .0f) {
            pose.speed = b.speed;
        } else if(b.speed == .0f) {
            pose.speed = a.speed;
        } else {
            pose.speed = gfxm::lerp(a.speed, b.speed, w);
        }

        pose.sample_buffer.getRootMotionDelta().t = gfxm::lerp(a.sample_buffer.getRootMotionDelta().t, b.sample_buffer.getRootMotionDelta().t, w);
    }

    void onGui() override;

    void write(out_stream& out) override {
        DataWriter w(&out);
        w.write<uint32_t>(blend_root_bones.size());
        for(int i = 0; i < blend_root_bones.size(); ++i) {
            w.write(blend_root_bones[i]);
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        uint32_t count = r.read<uint32_t>();
        for(int i = 0; i < count; ++i) {
            blend_root_bones.push_back(r.readStr());
        }
    }
};

class Blend3Job : public JobNode<Blend3Job, BlendTree> {
    Pose pose;
public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override {
        float w = get<float>(3);
        w = gfxm::_max(.0f, gfxm::_min(1.0f, w));
        w *= 2.0f;

        int left_idx = w;
        int right_idx = left_idx + 1;
        float lr_weight = w - (float)left_idx;

        Pose& a = get<Pose>(0);
        Pose& b = get<Pose>(1);
        Pose& c = get<Pose>(2);
        const Pose* array[] = {
            &a, &b, &c
        };
        if (left_idx == 2) {
            pose = c;
            return;
        }

        const Pose* _a = array[left_idx];
        const Pose* _b = array[right_idx];
        if (_a->sample_buffer.sampleCount() == 0 || _b->sample_buffer.sampleCount() == 0) {
            return;
        }

        for(size_t i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
            pose.sample_buffer[i].t = gfxm::lerp(_a->sample_buffer[i].t, _b->sample_buffer[i].t, lr_weight);
            pose.sample_buffer[i].r = gfxm::slerp(_a->sample_buffer[i].r, _b->sample_buffer[i].r, lr_weight);
            pose.sample_buffer[i].s = gfxm::lerp(_a->sample_buffer[i].s, _b->sample_buffer[i].s, lr_weight);
        }
        pose.speed = gfxm::lerp(_a->speed, _b->speed, lr_weight);

        pose.sample_buffer.getRootMotionDelta().t = gfxm::lerp(_a->sample_buffer.getRootMotionDelta().t, _b->sample_buffer.getRootMotionDelta().t, lr_weight);
    }
};


class SingleAnimJob : public JobNode<SingleAnimJob, BlendTree> {
    std::shared_ptr<Animation> anim;
    std::vector<int32_t> mapping;
    Pose pose;
    bool ready = false;

    void tryInit();

public:
    SingleAnimJob();

    void onInit(BlendTree*) override;
    void onInvoke() override;

    void onGui() override;

    void write(out_stream& out) override {
        DataWriter w(&out);
        if(anim) {
            w.write(anim->Name());
        } else {
            w.write(std::string());
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        std::string anim_name = r.readStr();
        anim = retrieve<Animation>(anim_name);
    }
};

class SingleAnimPoseJob : public JobNode<SingleAnimPoseJob, BlendTree> {
    std::shared_ptr<Animation> anim;
    std::vector<int32_t>       mapping;
    Pose                       pose;
    bool                       ready = false;

    void tryInit();

public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override;

    void onGui() override;

    void write(out_stream& out) override {
        DataWriter w(&out);
        if(anim) {
            w.write(anim->Name());
        } else {
            w.write(std::string());
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        std::string anim_name = r.readStr();
        anim = retrieve<Animation>(anim_name);
    }
};


class PoseResultJob : public JobNode<PoseResultJob, BlendTree> {
public:
    void onInit(BlendTree*) override {}
    void onInvoke() override;
};


class FloatNode : public JobNode<FloatNode, BlendTree> {
    float v = .5f;
    std::string value_name;
    int value_index = -1;
public:
    void onInit(BlendTree* bt);
    void onInvoke();

    void onGui();

    void write(out_stream& out) override {

    }
    void read(in_stream& in) override {
        
    }
};


class MotionParam : public JobNode<MotionParam, BlendTree> {
    float v = .0f;
    std::string param_name = "";
    int index = -1;
public:
    void onInit(BlendTree* bt);
    void onInvoke();

    void onOverride() override {
        if(index == -1) {
            return;
        }
        graph->getMotion()->getBlackboard().setValue(index, v);
    }

    void onGui();

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};



#endif
