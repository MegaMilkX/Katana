#include "node_graph.hpp"

#include <map>
#include <string>

void JobGraph::clear() {
    for(auto j : nodes) {
        delete j;
    }
    nodes.clear();
    nodes_by_uid.clear();
    nodes_by_type.clear();
}

void JobGraph::reinitNodes() {
    for(auto job : nodes) {
        job->init(this);
    }
}

std::set<JobGraphNode*>& JobGraph::getNodes() {
    return nodes;
}

bool JobGraph::isCompatible(JobGraphNode* job) {
    rttr::type graph_type = job->getGraphType();
    if(graph_type != rttr::type::get<JobGraph>()) {
        if(graph_type != getGraphType()) {
            return false;
        }
    }
    return true;
}

// Ownership is transferred to the JobGraph
void JobGraph::addNode(JobGraphNode* job) {
    //assert(job);

    if(!isCompatible(job)) {
        // TODO: delete job or forego ownership of jobs altogether
        LOG_WARN("JobNode " << job->get_type().get_name().to_string() << " is incompatible with graph");
        return;
    }

    job->setUid(next_uid++);
    nodes.insert(job);
    nodes_by_uid[job->getUid()] = job;
    nodes_by_type[job->get_type_id()].push_back(job);
    job->init(this);
    dirty = true;
}

JobGraphNode* JobGraph::getNode(uint32_t uid) {
    auto it = nodes_by_uid.find(uid);
    if(it == nodes_by_uid.end()) {
        return 0;
    }
    return it->second;
}

void JobGraph::prepare() {
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

void JobGraph::run() {
    if(dirty) {
        prepare();
    }
    for(auto job : invokable_nodes) {
        job->invoke();
    }
}

void JobGraph::write(out_stream& out) {
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
void JobGraph::read(in_stream& in) {
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
        j->init(this);
    }

    prepare();
}