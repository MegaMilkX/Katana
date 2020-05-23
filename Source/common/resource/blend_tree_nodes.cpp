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
    regJobNode<PoseResultJob>("PoseResult")
        .in<Pose>("result");
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