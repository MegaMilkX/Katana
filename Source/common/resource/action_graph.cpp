#include "action_graph.hpp"

#include "resource_tree.hpp"

static void buildAnimSkeletonMapping(Animation* anim, Skeleton* skel, std::vector<int32_t>& bone_mapping) {
    if(!anim || !skel) return;

    bone_mapping = std::vector<int32_t>(anim->nodeCount(), -1);
    for(size_t i = 0; i < skel->boneCount(); ++i) {
        auto& bone = skel->getBone(i);
        int32_t bone_index = (int32_t)i;
        int32_t node_index = anim->getNodeIndex(bone.name);
        if(node_index < 0) continue;
        
        bone_mapping[node_index] = bone_index;
    }
}

std::string pickUnusedName(const std::set<std::string>& names, const std::string& name) {
    std::string result = name;
    for(auto& a : names) {
        if(a == name) {
            std::string action_name = a;
            size_t postfix_pos = std::string::npos;
            for(int i = action_name.size() - 1; i >= 0; --i) {
                if((action_name[i] >= '0') && (action_name[i] <= '9')) {
                    postfix_pos = i;
                } else {
                    break;
                }
            }
            
            if(postfix_pos == std::string::npos) {
                action_name = action_name + "_1";
            } else {
                std::string postfix = action_name.substr(postfix_pos);
                action_name = action_name.substr(0, postfix_pos);
                int postfix_int = std::stoi(postfix);
                action_name = action_name + std::to_string(postfix_int + 1);
            }
            
            result = pickUnusedName(names, action_name);

            break;
        }
    }
    return result;
}

std::string pickUnusedName(const std::vector<ActionGraphNode*>& actions, const std::string& name) {
    std::set<std::string> names;
    for(auto& a : actions) {
        names.insert(a->getName());
    }
    return pickUnusedName(names, name);
}

void ActionGraphNode::update(
    float dt, 
    std::vector<AnimSample>& samples, 
    const std::map<Animation*, std::vector<int32_t>>& mappings,
    float weight
) {
    if(!anim) {
        return;
    }
    auto& it = mappings.find(anim.get());
    if(it == mappings.end()) {
        return;
    }

    anim->blend_remapped(samples, cursor, weight, it->second);

    cursor += dt * anim->fps;
    if(cursor > anim->length) {
        cursor -= anim->length;
    }
}

void ActionGraphNode::makeMappings(Skeleton* skel, std::map<Animation*, std::vector<int32_t>>& mappings) {
    if(!anim) {
        return;
    }
    buildAnimSkeletonMapping(anim.get(), skel, mappings[anim.get()]);
}

void ActionGraph::pickEntryAction() {
    if(actions.empty()) {
        return;
    }
    entry_action = 0;
    current_action = entry_action;
}

ActionGraphNode* ActionGraph::createAction(const std::string& name) {
    ActionGraphNode* node = new ActionGraphNode();
    node->setName(pickUnusedName(actions, name));
    actions.emplace_back(node);

    pickEntryAction();

    return node;
}

void ActionGraph::renameAction(ActionGraphNode* action, const std::string& name) {
    action->setName(pickUnusedName(actions, name));
}

ActionGraphTransition* ActionGraph::createTransition(const std::string& from, const std::string& to) {
    ActionGraphNode* a = findAction(from);
    ActionGraphNode* b = findAction(to);
    if(!a || !b) return 0;

    for(auto& t : transitions) {
        if(t->from == a && t->to == b) {
            return t;
        }
    }

    ActionGraphTransition* trans = new ActionGraphTransition{
        0.1f, a, b
    };
    transitions.emplace_back(trans);

    a->getTransitions().insert(trans);

    return trans;
}

ActionGraphNode* ActionGraph::findAction(const std::string& name) {
    ActionGraphNode* node = 0;
    for(auto& a : actions) {
        if(name == a->getName()) {
            node = a;
            break;
        }
    }
    return node;
}
int32_t ActionGraph::findActionId(const std::string& name) {
    ActionGraphNode* node = 0;
    for(size_t i = 0; i < actions.size(); ++i) {
        if(name == actions[i]->getName()) {
            return i;
        }
    }
    return -1;
}

void ActionGraph::deleteAction(ActionGraphNode* action) {
    std::set<size_t> transitions_to_remove;
    for(size_t i = 0; i < actions.size(); ++i) {
        auto& a = actions[i];
        if(a == action) {
            delete a;
            actions.erase(actions.begin() + i);
            
            for(auto it = transitions.begin(); it != transitions.end(); ) {
                if((*it)->from == action || (*it)->to == action) {
                    it = transitions.erase(it);
                } else {
                    ++it;
                }
            }

            break;
        }
    }

    if(entry_action >= actions.size()) {
        pickEntryAction();
    }
}
void ActionGraph::deleteTransition(ActionGraphTransition* transition) {
    for(size_t i = 0; i < transitions.size(); ++i) {
        auto& t = transitions[i];
        if(t == transition) {
            t->from->getTransitions().erase(t);
            delete t;
            transitions.erase(transitions.begin() + i);
            break;
        }
    }
}

const std::vector<ActionGraphNode*>&       ActionGraph::getActions() const {
    return actions;
}
const std::vector<ActionGraphTransition*>& ActionGraph::getTransitions() const {
    return transitions;
}

size_t ActionGraph::getEntryActionId() {
    return entry_action;
}
ActionGraphNode* ActionGraph::getEntryAction() {
    if(entry_action < 0) return 0;
    return actions[entry_action];
}
void ActionGraph::setEntryAction(const std::string& name) {
    entry_action = findActionId(name);
    current_action = entry_action;
}

ActionGraphParams& ActionGraph::getParams() {
    return param_table;
}

void ActionGraph::update(
    float dt, 
    std::vector<AnimSample>& samples, 
    const std::map<Animation*, std::vector<int32_t>>& mappings
) {
    if(current_action > actions.size()) {
        return;
    }
    auto act = actions[current_action];

    for(auto& t : act->getTransitions()) {
        bool res = false;
        for(auto& cond : t->conditions) {
            float val = param_table.getParam(cond.param).value;
            float ref_val = cond.ref_value;
            switch(cond.type) {
            case ActionGraphTransition::LARGER: res = val > ref_val; break;
            case ActionGraphTransition::LARGER_EQUAL: res = val >= ref_val; break;
            case ActionGraphTransition::LESS: res = val < ref_val; break;
            case ActionGraphTransition::LESS_EQUAL: res = val <= ref_val; break;
            case ActionGraphTransition::EQUAL: res = val == ref_val; break;
            case ActionGraphTransition::NOT_EQUAL: res = val != ref_val; break;
            };
            if(res == false) {
                break;
            }
        }
        if(res) {
            for(size_t i = 0; i < actions.size(); ++i) {
                if(actions[i] == t->to) {
                    current_action = i;
                }
            }
            act = actions[current_action];
            trans_samples = samples;
            trans_speed = 1.0f / t->blendTime;
            trans_weight = .0f;
        }
    }

    if(trans_weight < 1.0f) {
        samples = trans_samples;
    }
    if(trans_weight < 1.0f) {
        trans_weight += dt * trans_speed;
        if(trans_weight > 1.0f) {
            trans_weight = 1.0f;
        }
    }
    act->update(dt, samples, mappings, trans_weight);
}

void ActionGraph::makeMappings(Skeleton* skel, std::map<Animation*, std::vector<int32_t>>& mappings) {
    for(auto& a : actions) {
        a->makeMappings(skel, mappings);
    }
}

void ActionGraph::serialize(out_stream& out) {
    DataWriter w(&out);

    std::map<ActionGraphNode*, uint32_t> node_ids;
    std::map<ActionGraphTransition*, uint32_t> trans_ids;
    for(size_t i = 0; i < actions.size(); ++i) {
        node_ids[actions[i]] = (uint32_t)i;
    }
    for(size_t i = 0; i < transitions.size(); ++i) {
        trans_ids[transitions[i]] = (uint32_t)i;
    }

    out.write<uint32_t>(actions.size());
    out.write<uint32_t>(transitions.size());
    out.write<uint32_t>(param_table.paramCount());
    out.write<uint32_t>(entry_action);
    for(size_t i = 0; i < actions.size(); ++i) {
        auto& a = actions[i];
        w.write(a->getName());
        w.write(a->getEditorPos());
        if(a->anim) {
            w.write(a->anim->Name());
        } else {
            w.write(std::string());
        }
    }
    for(size_t i = 0; i < transitions.size(); ++i) {
        auto& t = transitions[i];
        w.write(t->blendTime);
        w.write(node_ids[t->from]);
        w.write(node_ids[t->to]);

        w.write<uint32_t>(t->conditions.size());
        for(size_t j = 0; j < t->conditions.size(); ++j) {
            auto& cond = t->conditions[j];
            w.write<uint32_t>(cond.param);
            w.write<uint8_t>(cond.type);
            w.write<float>(cond.ref_value);
        }
    }
    for(size_t i = 0; i < param_table.paramCount(); ++i) {
        auto& param = param_table.getParam(i);
        w.write(param.name);
        w.write(param.value);
    }
}
bool ActionGraph::deserialize(in_stream& in, size_t sz) {
    DataReader r(&in);

    actions.resize(r.read<uint32_t>());
    transitions.resize(r.read<uint32_t>());
    param_table.resize(r.read<uint32_t>());
    for(size_t i = 0; i < actions.size(); ++i) {
        actions[i] = new ActionGraphNode();
    }
    for(size_t i = 0; i < transitions.size(); ++i) {
        transitions[i] = new ActionGraphTransition();
    }
    entry_action = r.read<uint32_t>();
    current_action = entry_action;

    for(size_t i = 0; i < actions.size(); ++i) {
        auto& a = actions[i];
        a->setName(r.readStr());
        a->setEditorPos(r.read<gfxm::vec2>());
        std::string anim_name = r.readStr();
        a->anim = retrieve<Animation>(anim_name);
    }
    for(size_t i = 0; i < transitions.size(); ++i) {
        auto& t = transitions[i];
        t->blendTime = r.read<float>();
        t->from = actions[r.read<uint32_t>()];
        t->from->getTransitions().insert(t);
        t->to = actions[r.read<uint32_t>()];

        t->conditions.resize(r.read<uint32_t>());
        for(size_t j = 0; j < t->conditions.size(); ++j) {
            auto& cond = t->conditions[j];
            cond.param = (size_t)r.read<uint32_t>();
            cond.type = (ActionGraphTransition::CONDITION)r.read<uint8_t>();
            cond.ref_value = r.read<float>();
        }
    }
    for(size_t i = 0; i < param_table.paramCount(); ++i) {
        auto& param = param_table.getParam(i);
        param.name = r.readStr();
        param.value = r.read<float>();
    }

    return true;
}
