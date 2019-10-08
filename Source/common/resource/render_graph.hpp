#ifndef RENDER_GRAPH_HPP
#define RENDER_GRAPH_HPP

#include "resource.h"
#include "../util/func_graph/node_graph.hpp"

#include "../gl/frame_buffer.hpp"

class RenderJobTexture2d : public JobNode<RenderJobTexture2d> {
    std::shared_ptr<Texture2D> texture;
    GLuint texid;
public:
    void onInit() {
        bind<GLuint>(&texid);
    }
    void onInvoke() {
        if(!texture) return;

        texid = texture->GetGlName();
    }
    void onGui() {
        imguiResourceTreeCombo("texture", texture, "png");
    }
};

class RenderJobFrameBuffer : public JobNode<RenderJobFrameBuffer> {
    gl::FrameBuffer fb;
    gl::FrameBuffer* fb_ptr = &fb;
public:
    void onInit() {
        fb.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE);
        fb.reinitBuffers(640, 480);
        bind<gl::FrameBuffer*>(&fb_ptr);
    }
    void onInvoke() {

    }

    void onGui() {

    }
};

class RenderJobClear : public JobNode<RenderJobClear> {
    gl::FrameBuffer* fb;
    gfxm::vec4 color;
public:
    void onInit() {
        bind(&fb);
    }
    void onInvoke() {
        fb = get<gl::FrameBuffer*>(0);
        fb->bind();
        glClearColor(color.x, color.y, color.z, color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void onGui() {
        ImGui::ColorEdit4("color", (float*)&color);
    }
};

class RenderJobRoot : public JobNode<RenderJobRoot> {
    gl::FrameBuffer* fb = 0;
public:
    void onInit() {

    }
    void onInvoke() {
        fb = get<gl::FrameBuffer*>(0);
    }
    void onGui() {

    }

    gl::FrameBuffer* getFrameBuffer() {
        return get<gl::FrameBuffer*>(0);
    }
};

template<typename ROOT_T>
class JobTree : public JobGraph {
    ROOT_T* rootNode;

public:
    JobTree() {
        rootNode = new ROOT_T;
        addNode(rootNode);
    }

    void clear() override {
        clear();
        rootNode = new ROOT_T;
        addNode(rootNode);
    }

    ROOT_T* getRoot() { return rootNode; }

    void write(out_stream& out) override {
        DataWriter w(&out);
        
        std::map<JobGraphNode*, uint32_t> node_id_map;
        std::map<JobOutput*, uint32_t> out_id_map;
        std::vector<JobInput*> inputs;
        for(auto j : nodes) {
            uint32_t id = node_id_map.size();
            node_id_map[j] = id;
            for(size_t i = 0; i < j->inputCount(); ++i) {
                inputs.emplace_back(j->getInput(i));
            }
            for(size_t i = 0; i < j->outputCount(); ++i) {
                uint32_t out_id = out_id_map.size();
                out_id_map[j->getOutput(i)] = out_id;
            }
        }

        w.write(next_uid);
        w.write<uint32_t>(nodes.size());
        for(auto j : nodes) {
            w.write(j->getDesc().name);
            w.write(j->getUid());
            w.write(j->getPos());
        }

        w.write<uint32_t>(inputs.size());
        for(size_t i = 0; i < inputs.size(); ++i) {
            JobInput* in = inputs[i];
            if(in->source) {
                w.write<uint32_t>(out_id_map[in->source]);
            } else {
                w.write<uint32_t>(-1);
            }
        }
    }
    virtual void read(in_stream& in) override {
        DataReader r(&in);

        std::vector<JobGraphNode*> nodes_tmp;
        std::vector<JobInput*> ins_tmp;
        std::vector<JobOutput*> outs_tmp;

        next_uid = r.read<uint32_t>();
        uint32_t node_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < node_count; ++i) {
            std::string node_name = r.readStr();
            JobGraphNode* node = 0;
            if(i == 0) {
                node = *nodes.begin();
            } else {
                node = createJobNode(node_name);
            }
            node->setUid(r.read<uint32_t>());
            node->setPos(r.read<gfxm::vec2>());
            nodes_tmp.emplace_back(
                node
            );
            for(size_t j = 0; j < node->inputCount(); ++j) {
                ins_tmp.emplace_back(node->getInput(j));
            }
            for(size_t j = 0; j < node->outputCount(); ++j) {
                outs_tmp.emplace_back(node->getOutput(j));
            }
        }

        uint32_t input_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < input_count; ++i) {
            uint32_t out_id = r.read<uint32_t>();
            if(out_id != (uint32_t)-1) {
                ins_tmp[i]->source = outs_tmp[out_id];
            }
        }


        for(auto j : nodes_tmp) {
            nodes.insert(j);
            nodes_by_uid[j->getUid()] = j;
            j->init();
        }

        prepare();
    }
};

class RenderGraph : public Resource, public JobTree<RenderJobRoot> {
    RTTR_ENABLE(Resource)

public:
    void run() override {
        glViewport(0, 0, 640, 480);
        glDisable(GL_SCISSOR_TEST);
        JobGraph::run();
    }

    void serialize(out_stream& out) {}
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
};
STATIC_RUN(RenderGraph) {
    rttr::registration::class_<RenderGraph>("RenderGraph")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}
STATIC_RUN(RENDER_JOBS) {
    regJobNode<RenderJobRoot>("Result")
        .in<gl::FrameBuffer*>("in")
        .color(.8f, .4f, .1f);
    regJobNode<RenderJobTexture2d>("texture2d")
        .out<GLuint>("id")
        .color(.4f, .1f, .4f);
    regJobNode<RenderJobFrameBuffer>("FrameBuffer")
        .out<gl::FrameBuffer*>("framebuffer")
        .color(.4f, .1f, .4f);
    regJobNode<RenderJobClear>("Clear")
        .out<gl::FrameBuffer*>("out")
        .in<gl::FrameBuffer*>("in")
        .color(.0f, .4f, .1f);
}

#endif
