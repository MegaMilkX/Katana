#include "action_graph.hpp"

#include "resource_tree.hpp"

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

ActionGraphNode::ActionGraphNode() {
    motion.reset(new ClipMotion());
}

void ActionGraphNode::update(
    float dt, 
    std::vector<AnimSample>& samples,
    float weight
) {
    auto& pose = motion->getPose();
    motion->advance(dt);

    for(size_t i = 0; i < pose.size() && i < samples.size(); ++i) {
        samples[i] = pose[i];
    }
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


void ActionGraph::update(
    float dt, 
    std::vector<AnimSample>& samples
) {
    if(!blackboard) {
        assert(false);
    }
    if(current_action > actions.size()) {
        return;
    }
    auto act = actions[current_action];

    for(auto& t : act->getTransitions()) {
        bool res = false;
        for(auto& cond : t->conditions) {
            float val = blackboard->get_float(cond.param_hdl);
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

    
    act->update(dt, samples, trans_weight);
    if(trans_weight < 1.0f) {
        for(size_t i = 0; i < samples.size(); ++i) {
            samples[i].t = gfxm::lerp(trans_samples[i].t, samples[i].t, trans_weight);
            samples[i].r = gfxm::slerp(trans_samples[i].r, samples[i].r, trans_weight);
            samples[i].s = gfxm::lerp(trans_samples[i].s, samples[i].s, trans_weight);
        }

        trans_weight += dt * trans_speed;
        if(trans_weight > 1.0f) {
            trans_weight = 1.0f;
        }
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
    out.write<uint32_t>(entry_action);
    for(size_t i = 0; i < actions.size(); ++i) {
        auto& a = actions[i];
        w.write(a->getName());
        w.write(a->getEditorPos());
        w.write<uint8_t>((uint8_t)a->motion->getType());
        a->motion->write(out);
    }
    for(size_t i = 0; i < transitions.size(); ++i) {
        auto& t = transitions[i];
        w.write(t->blendTime);
        w.write(node_ids[t->from]);
        w.write(node_ids[t->to]);

        w.write<uint32_t>(t->conditions.size());
        for(size_t j = 0; j < t->conditions.size(); ++j) {
            auto& cond = t->conditions[j];
            w.write(std::string(cond.param_name)); // Blackboard value name
            w.write<uint8_t>(cond.type);
            w.write<float>(cond.ref_value);
        }
    }

    if(reference_object) {
        w.write(reference_object->Name());
    } else {
        w.write(std::string());
    }
    if(reference_skel) {
        w.write(reference_skel->Name());
    } else {
        w.write(std::string());
    }
}
bool ActionGraph::deserialize(in_stream& in, size_t sz) {
    DataReader r(&in);

    actions.resize(r.read<uint32_t>());
    transitions.resize(r.read<uint32_t>());
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
        MOTION_TYPE motion_type = (MOTION_TYPE)r.read<uint8_t>();
        if(motion_type == MOTION_CLIP) {
            a->motion.reset(new ClipMotion());
        } else if(motion_type == MOTION_BLEND_TREE) {
            a->motion.reset(new BlendTreeMotion());
        }
        if(a->motion) {
            a->motion->read(in);
        }
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
            cond.param_name = r.readStr();
            cond.type = (ActionGraphTransition::CONDITION)r.read<uint8_t>();
            cond.ref_value = r.read<float>();
        }
    }

    std::string ref_name = r.readStr();
    std::string skl_name = r.readStr();
    reference_object = retrieve<GameScene>(ref_name);
    reference_skel = retrieve<Skeleton>(skl_name);

    return true;
}
