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

void SingleAnimJob::setBlendTree(BlendTree* bt) {
    blendTree = bt;
}

void SingleAnimJob::setSkeleton(std::shared_ptr<Skeleton> skel) {
    this->skel = skel;
    if(!skel) return;
    pose.samples = skel->makePoseArray();
}

void SingleAnimJob::onInit() {
    bind<Pose>(&pose);
}
void SingleAnimJob::onInvoke() {
    if(!skel || !anim || !blendTree) return;

    std::vector<int32_t>& mapping = anim->getMapping(skel.get());
    anim->sample_remapped(pose.samples, blendTree->getCursor() * anim->length, mapping);
    pose.speed = anim->fps / anim->length;
}

void SingleAnimJob::onGui() {
    imguiResourceTreeCombo("anim clip", anim, "anm", [](){

    });
}
