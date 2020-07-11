#include "blend_tree.hpp"

#include "blend_tree.hpp"

#include "motion.hpp"

STATIC_RUN(BLEND_TREE_NODES) {
    regJobNode<TestJob>("Test")
        .out<float>("value");
    regJobNode<MultiplyJob>("Multiply")
        .in<float>("a")
        .in<float>("b")
        .out<float>("result");
    regJobNode<PrintJob>("Print")
        .in<float>("result");

    regJobNode<SingleAnimJob>("SingleAnim")
        .out<Pose>("pose")
        .color(0.0, 0.4, 0.1);
    regJobNode<SingleAnimPoseJob>("SingleAnimPose")
        .in<float>("normal cursor")
        .out<Pose>("pose")
        .color(.0, .4, .1);
    regJobNode<PoseResultJob>("PoseResult")
        .in<Pose>("result");
    regJobNode<BlendAddJob>("BlendAdd")
        .in<Pose>("target")
        .in<Pose>("pose")
        .in<Pose>("reference")
        .out<Pose>("result")
        .color(.4f, .0f, .2f);
    regJobNode<Blend2Job>("Blend2")
        .in<Pose>("pose A")
        .in<Pose>("pose B")
        .in<float>("weight")
        .out<Pose>("result")
        .color(.1, .0, .4);
    regJobNode<Blend3Job>("Blend3")
        .in<Pose>("pose0")
        .in<Pose>("pose1")
        .in<Pose>("pose2")
        .in<float>("weight")
        .out<Pose>("result")
        .color(0.1, 0.0, 0.4);
    regJobNode<FloatNode>("Float")
        .out<float>("float")
        .color(0.4, 0.0, 0.0);
    regJobNode<MotionParam>("MotionParam")
        .out<float>("value")
        .color(0.4, .0f, .0f);
}


void BlendAddJob::onInit(BlendTree* bt) {
    bind<Pose>(&pose);
    if (!bt->getMotion()->getSkeleton()) {
        return;
    }
    pose.sample_buffer = AnimSampleBuffer(bt->getMotion()->getSkeleton().get());
}


void Blend2Job::reinit() {
    if(!graph->getMotion()->getSkeleton()) {
        return;
    }
    filter_weights.clear();
    pose.sample_buffer = AnimSampleBuffer(graph->getMotion()->getSkeleton().get());
    
    if(!blend_root_bones.empty()) {
        filter_weights = std::vector<float>(pose.sample_buffer.sampleCount(), .0f);
        
        for(int i = 0; i < blend_root_bones.size(); ++i) {
            auto& blend_root_bone = blend_root_bones[i];
            auto b = graph->getMotion()->getSkeleton()->getBone(blend_root_bone);
            if(b) {
                std::function<void(int32_t, Skeleton*)> walk_bones_fn = [this, &walk_bones_fn](int32_t bi, Skeleton* sk){
                    filter_weights[bi] = 1.0f;
                    auto& b = sk->getBone(bi);
                    auto child = b.first_child;
                    while(child != -1) {
                        auto& b = sk->getBone(child);
                        walk_bones_fn(child, sk);
                        child = b.next_sibling;
                    }
                };
                walk_bones_fn(b->id, graph->getMotion()->getSkeleton().get());
            }
        }
    } else {
        filter_weights = std::vector<float>(pose.sample_buffer.sampleCount(), 1.0f);
    }
}
void Blend2Job::onInit(BlendTree* bt) {
    bind<Pose>(&pose);
    reinit();
}
void Blend2Job::onGui() {
    auto skel = this->graph->getMotion()->getSkeleton();
    if(!skel) {
        ImGui::Text("!Missing skeleton!");
        return;
    }
    
    std::function<void(Skeleton*, int32_t)> walk_bones_fn = [&walk_bones_fn, this](Skeleton* sk, int32_t id){        
        auto& b = sk->getBone(id);
        auto child = b.first_child;

        if(child == -1) {
            if(ImGui::Selectable(sk->getBone(id).name.c_str())) {
                blend_root_bones.push_back(sk->getBone(id).name);
                reinit();
            }
        } else {
            ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
            //if(selected) {
            //    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
            //}
            bool open = ImGui::TreeNodeEx(sk->getBone(id).name.c_str(), tree_node_flags);
            if(ImGui::IsItemClicked(0)) {
                blend_root_bones.push_back(sk->getBone(id).name);
                reinit();
            }
            if(open) {
                while(child != -1) {
                    walk_bones_fn(sk, child);
                    auto& cb = sk->getBone(child);
                    child = cb.next_sibling;
                }
                ImGui::TreePop();
            }
        }        
    };
    if(ImGui::BeginCombo("bones", "Select bones...")) {
        for(int i = 0; i < skel->rootBoneCount(); ++i) {
            auto& b = skel->getRootBone(i);
            walk_bones_fn(skel.get(), b.id);
        }
        ImGui::EndCombo();
    }
    if(ImGui::ListBoxHeader("affected bones")) {
        for(int i = 0; i < filter_weights.size(); ++i) {
            if(filter_weights[i] == .0f) {
                continue;
            }
            if(ImGui::Selectable(skel->getBone(i).name.c_str())) {

            }
        }
        ImGui::ListBoxFooter();
    }
    if(ImGui::Button("Clear bone filter")) {
        filter_weights = std::vector<float>(filter_weights.size(), .0f);
        blend_root_bones.clear();
    }
    if(ImGui::Button("Add all")) {
        filter_weights = std::vector<float>(filter_weights.size(), 1.0f);
    }
}


void Blend3Job::onInit(BlendTree* bt) {
    bind<Pose>(&pose);
    if (!bt->getMotion()->getSkeleton()) {
        return;
    }
    pose.sample_buffer = AnimSampleBuffer(bt->getMotion()->getSkeleton().get());
}


SingleAnimJob::SingleAnimJob() {
}

void SingleAnimJob::onInit(BlendTree* bt) {
    bind<Pose>(&pose);

    tryInit();
}
void SingleAnimJob::onInvoke() {
    if(!ready) return;

    anim->sample_remapped(pose.sample_buffer, graph->getPrevCursor() * anim->length, graph->getCursor() * anim->length, graph->getMotion()->getSkeleton().get(), mapping);
    pose.speed = anim->fps / anim->length;
}

void SingleAnimJob::onGui() {
    imguiResourceTreeCombo("anim clip", anim, "anm", [this](){
        tryInit();
    });
}

void SingleAnimJob::tryInit() {
    auto skel = graph->getMotion()->getSkeleton();
    if(skel) {
        pose.sample_buffer = AnimSampleBuffer(skel.get());
    }
    if(skel && anim) {
        mapping = anim->getMapping(skel.get());

        ready = true;
    } else {
        ready = false;
    }
}


void SingleAnimPoseJob::onInit(BlendTree* bt) {
    bind<Pose>(&pose);
    tryInit();
}
void SingleAnimPoseJob::onInvoke() {
    if(!ready) return;

    float c = get<float>(0);

    anim->sample_remapped(pose.sample_buffer, c * anim->length, c * anim->length, graph->getMotion()->getSkeleton().get(), mapping);
    pose.speed = .0f;
}
void SingleAnimPoseJob::onGui() {
    imguiResourceTreeCombo("anim clip", anim, "anm", [this](){
        tryInit();
    });
}
void SingleAnimPoseJob::tryInit() {
    auto skel = graph->getMotion()->getSkeleton();
    if(skel) {
        pose.sample_buffer = AnimSampleBuffer(skel.get());
    }
    if(skel && anim) {
        mapping = anim->getMapping(skel.get());

        ready = true;
    } else {
        ready = false;
    }
}


void PoseResultJob::onInvoke() {
    auto& pose = get<Pose>(0);
    graph->_reportPose(pose);
}


void FloatNode::onInit(BlendTree* bt) {
    bind<float>(&v);
}

void FloatNode::onInvoke() {
}   

void FloatNode::onGui() {
    ImGui::DragFloat("value", &v, 0.01f);
}

#include "motion.hpp"

void MotionParam::onInit(BlendTree* bt) {
    bind<float>(&v);

    if(!param_name.empty()) {
        index = graph->getMotion()->getBlackboard().getIndex(param_name.c_str());
        if (index < 0) {
            index = graph->getMotion()->getBlackboard().allocValue();
            graph->getMotion()->getBlackboard().setName(index, param_name.c_str());
        }
    }
}
void MotionParam::onInvoke() {
    if(index < 0) {
        return;
    }
    v = graph->getMotion()->getBlackboard().getValue(index);
}
void MotionParam::onGui() {
    std::string preview_name = "<null>";
    if(!param_name.empty()) {
        preview_name = param_name;
    }
    if(ImGui::BeginCombo(MKSTR("param###cond_id").c_str(), preview_name.c_str(), ImGuiComboFlags_NoArrowButton)) {
        for(auto it = graph->getMotion()->getBlackboard().begin(); it != false; ++it) {
            if(ImGui::Selectable((*it).name.c_str(), index == it.getIndex())) {
                index = it.getIndex();
                param_name = (*it).name;
            }
        }
        ImGui::EndCombo();
    } 
}

void MotionParam::write(out_stream& out) {
    DataWriter w(&out);
    w.write(param_name);
}
void MotionParam::read(in_stream& in) {
    DataReader r(&in);
    param_name = r.readStr();
}