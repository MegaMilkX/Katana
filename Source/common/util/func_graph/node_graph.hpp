#ifndef FUNC_NODE_GRAPH_HPP
#define FUNC_NODE_GRAPH_HPP

#include <tuple>
#include <type_traits>
#include <stack>

#include "../log.hpp"

#include "job_node_desc_lib.hpp"

inline uint64_t next_job_node_type_id() {
    static uint64_t id = 0;
    return id++;
}

template<typename NODE_T>
class JobNode : public JobGraphNode {
protected:
    template<typename T>
    T& get(size_t i) {
        JobInput& in = inputs[i];
        return *(T*)in.source->value_ptr;
    }
    template<typename T>
    void bind(T* value_ptr) {
        bind(0, value_ptr);
    }
    template<typename T>
    void bind(size_t i, T* value_ptr) {
        JobOutput& out = outputs[i];
        out.value_ptr = value_ptr;
    }

    virtual void onInvoke() = 0;

public:
    JobNode() {
        desc = JobNodeDescLib::get()->getDesc(rttr::type::get<NODE_T>());
        for(auto& in : desc->ins) {
            inputs.emplace_back(JobInput{ 0, in.type, this });
        }
        for(auto& out : desc->outs) {
            outputs.emplace_back(JobOutput{ 0, out.type, this });
        }
    }

    static uint64_t get_type_id_static() {
        static uint64_t id = next_job_node_type_id();
        return id;
    } 
    uint64_t get_type_id() const override {  
        return get_type_id_static();
    }

    bool isInvokable() override { return true; }

    void invoke() override {
        onInvoke();
    }    
};

class MultiplyJob : public JobNode<MultiplyJob> {
    float result = .0f;
public:
    void onInit() override {
        bind<float>(0, &result);
    }

    void onInvoke() override {
        auto a = get<float>(0);
        auto b = get<float>(1);

        result = a * b;
    }
};

class TestJob : public JobNode<TestJob> {
    float val = 100.0f;
public:
    void onInit() override {
        bind<float>(0, &val);
    }

    void onInvoke() override {
        val += 1.0f/60.0f;
    }
};

class PrintJob : public JobNode<PrintJob> {
public:
    void onInit() override {}

    void onInvoke() override {
        auto a = get<float>(0);
        LOG(a);
    }
};

#include "../data_stream.hpp"
#include "../data_writer.hpp"
#include "../data_reader.hpp"

class JobGraph {
protected:
    bool dirty = true;
    std::set<JobGraphNode*> nodes;
    std::map<uint32_t, JobGraphNode*> nodes_by_uid;
    std::map<uint64_t, std::vector<JobGraphNode*>> nodes_by_type;
    uint32_t next_uid = 0;
    std::vector<JobGraphNode*> invokable_nodes;
public:
    JobGraph() {}
    virtual ~JobGraph() {}

    virtual void clear() {
        for(auto j : nodes) {
            delete j;
        }
        nodes.clear();
        nodes_by_uid.clear();
        nodes_by_type.clear();
    }

    std::set<JobGraphNode*>& getNodes() {
        return nodes;
    }

    template<typename T>
    size_t countOf() {
        auto it = nodes_by_type.find(T::get_type_id_static());
        if(it == nodes_by_type.end()) {
            return 0;
        }
        return it->second.size();
    }

    template<typename T>
    T* getNode(size_t id) {
        auto it = nodes_by_type.find(T::get_type_id_static());
        if(it == nodes_by_type.end()) {
            return 0;
        }
        return (T*)it->second[id];
    }

    // Ownership is transferred to the JobGraph
    void addNode(JobGraphNode* job) {
        //assert(job);

        job->setUid(next_uid++);
        nodes.insert(job);
        nodes_by_uid[job->getUid()] = job;
        nodes_by_type[job->get_type_id()].push_back(job);
        job->init();
        dirty = true;
    }

    JobGraphNode* getNode(uint32_t uid) {
        auto it = nodes_by_uid.find(uid);
        if(it == nodes_by_uid.end()) {
            return 0;
        }
        return it->second;
    }

    void prepare() {
        invokable_nodes.clear();
        std::set<JobGraphNode*> valid_nodes;

        
        for(auto job : nodes) {
            if(!job->isInvokable()) {
                continue;
            }
            if(job->isValid()) { // Ignore jobs that lack inputs
                valid_nodes.insert(job);
                invokable_nodes.emplace_back(job);
            }
        }

        // Sort execution order
        std::map<JobGraphNode*, int> node_weights;
        for(auto n : valid_nodes) {
            std::stack<JobGraphNode*> stack;
            JobGraphNode* cur = n;
            while(cur || !stack.empty()) {
                if(!cur) {
                    cur = stack.top();
                    stack.pop();
                }

                node_weights[cur]++;

                for(size_t j = 0; j < cur->inputCount(); ++j) {
                    auto in_src = cur->getInput(j)->source->owner;
                    if(valid_nodes.count(in_src) == 0) {
                        continue;
                    }
                    if(!in_src) {
                        continue;
                    }

                    stack.push(in_src);
                }

                cur = 0;
            }
        }

        std::sort(invokable_nodes.begin(), invokable_nodes.end(), [&node_weights](JobGraphNode* a, JobGraphNode* b)->bool{
            return node_weights[a] > node_weights[b];
        });

        dirty = false;
    }

    virtual void run() {
        if(dirty) {
            prepare();
        }
        for(auto job : invokable_nodes) {
            job->invoke();
        }
    }

    virtual void write(out_stream& out) {
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
    virtual void read(in_stream& in) {
        DataReader r(&in);

        std::vector<JobGraphNode*> nodes_tmp;
        std::vector<JobInput*> ins_tmp;
        std::vector<JobOutput*> outs_tmp;

        next_uid = r.read<uint32_t>();
        uint32_t node_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < node_count; ++i) {
            std::string node_name = r.readStr();
            auto node = createJobNode(node_name);
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
            nodes_by_type[j->get_type_id()].push_back(j);
            j->init();
        }

        prepare();
    }
};


#endif
