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
    regJobNode<MotionParam>("MotionParam")
        .out<float>("value")
        .color(0.4, .0f, .0f);
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


void FloatNode::onInit(BlendTree* bt) {
    bind<float>(&v);

    //value_index = bt->getValueIndex(value_name.c_str());
    value_index = bt->declValue(value_name.c_str());
}

void FloatNode::onInvoke() {
    if(value_index >= 0) {
        v = graph->getValue(value_index);
    }
}   

void FloatNode::onGui() {
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if(!value_name.empty()) {
        memcpy(buf, value_name.c_str(), value_name.size());
    }
    if (ImGui::InputText("value name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        value_name = buf;
        graph->compile();
    }
    /*
    if (ImGui::BeginCombo("value", value_name.c_str())) {
        for (int i = 0; i < graph->valueCount(); ++i) {
            std::string name = graph->getValueName(i);
            if (ImGui::Selectable(name.c_str(), value_name == name)) {
                value_name = name;
                value_index = graph->getValueIndex(value_name.c_str());
            }
        }

        ImGui::EndCombo();
    }*/
}

#include "motion.hpp"

void MotionParam::onInit(BlendTree* bt) {
    bind<float>(&v);
}
void MotionParam::onInvoke() {
    if(index < 0) {
        return;
    }
    v = graph->getMotion()->getBlackboard().getValue(index);
}
void MotionParam::onGui() {
    if(ImGui::BeginCombo(MKSTR("param###cond_id").c_str(), param_name.c_str(), ImGuiComboFlags_NoArrowButton)) {
        for(auto it = graph->getMotion()->getBlackboard().begin(); it != false; ++it) {
            if(ImGui::Selectable((*it).name.c_str(), index == it.getIndex())) {
                index = it.getIndex();
                param_name = (*it).name;
            }
        }
        ImGui::EndCombo();
    } 
}