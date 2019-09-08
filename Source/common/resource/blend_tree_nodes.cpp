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