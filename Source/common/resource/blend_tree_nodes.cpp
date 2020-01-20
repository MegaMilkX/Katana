#include "blend_tree.hpp"

#include "blend_tree.hpp"

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
}


SingleAnimJob::SingleAnimJob() {
}

void SingleAnimJob::onInit(BlendTree* bt) {
    bind<Pose>(&pose);

    tryInit();
}
void SingleAnimJob::onInvoke() {
    if(!ready) return;

    anim->sample_remapped(pose.samples, graph->getCursor() * anim->length, mapping);
    pose.speed = anim->fps / anim->length;
}

void SingleAnimJob::onGui() {
    imguiResourceTreeCombo("anim clip", anim, "anm", [this](){
        tryInit();
    });
}

void SingleAnimJob::tryInit() {
    auto skel = graph->getSkeleton();
    if(skel) {
        pose.samples = skel->makePoseArray();
    }
    if(skel && anim) {
        mapping = anim->getMapping(skel);

        ready = true;
    } else {
        ready = false;
    }
}


void PoseResultJob::onInvoke() {
    auto& pose = get<Pose>(0);
    graph->_reportPose(pose);
}