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

template<typename NODE_T, typename GRAPH_T>
class JobNode : public JobGraphNode {
protected:
    GRAPH_T* graph = 0; // Direct owner graph access for global data

    rttr::type getGraphType() const override { 
        return rttr::type::get<GRAPH_T>(); 
    }
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

    void onInit_(JobGraph* owner_graph) override {
        graph = (GRAPH_T*)owner_graph;
        onInit(graph);
    }
    virtual void onInit(GRAPH_T* owner_graph) = 0;
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

class MultiplyJob : public JobNode<MultiplyJob, JobGraph> {
    float result = .0f;
public:
    void onInit(JobGraph*) override {
        bind<float>(0, &result);
    }

    void onInvoke() override {
        auto a = get<float>(0);
        auto b = get<float>(1);

        result = a * b;
    }
};

class TestJob : public JobNode<TestJob, JobGraph> {
    float val = 100.0f;
public:
    void onInit(JobGraph*) override {
        bind<float>(0, &val);
    }

    void onInvoke() override {
        val += 1.0f/60.0f;
    }
};

class PrintJob : public JobNode<PrintJob, JobGraph> {
public:
    void onInit(JobGraph*) override {}

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

    virtual void clear();
    void reinitNodes();
    std::set<JobGraphNode*>& getNodes();

    template<typename T>
    size_t countOf();
    template<typename T>
    T* getNode(size_t id);

    virtual rttr::type getGraphType() const { return rttr::type::get<JobGraph>(); }

    bool isCompatible(JobGraphNode* job);

    // Ownership is transferred to the JobGraph
    void addNode(JobGraphNode* job);
    JobGraphNode* getNode(uint32_t uid);

    void prepare();

    virtual void run();

    virtual void write(out_stream& out);
    virtual void read(in_stream& in);
};

template<typename T>
size_t JobGraph::countOf() {
    auto it = nodes_by_type.find(T::get_type_id_static());
    if(it == nodes_by_type.end()) {
        return 0;
    }
    return it->second.size();
}

template<typename T>
T* JobGraph::getNode(size_t id) {
    auto it = nodes_by_type.find(T::get_type_id_static());
    if(it == nodes_by_type.end()) {
        return 0;
    }
    return (T*)it->second[id];
}


template<typename T>
class JobGraphTpl : public JobGraph {
public:
    rttr::type getGraphType() const override { return rttr::type::get<T>(); }
};


#endif
