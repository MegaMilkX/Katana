#include "anim_fsm.hpp"

#include "resource_tree.hpp"

#include "../util/make_next_name.hpp"

std::string pickUnusedName(const std::vector<AnimFSMState*>& actions, const std::string& name) {
    std::set<std::string> names;
    for(auto& a : actions) {
        names.insert(a->getName());
    }
    return makeNextName(names, name);
}



void AnimFSM::pickEntryAction() {
    if(actions.empty()) {
        return;
    }
    entry_action = 0;
    current_action = entry_action;
}

void AnimFSM::renameAction(AnimFSMState* action, const std::string& name) {
    action->setName(pickUnusedName(actions, name));
}

AnimFSMTransition* AnimFSM::createTransition(const std::string& from, const std::string& to) {
    AnimFSMState* a = findAction(from);
    AnimFSMState* b = findAction(to);
    if(!a || !b) return 0;

    for(auto& t : transitions) {
        if(t->from == a && t->to == b) {
            return t;
        }
    }

    AnimFSMTransition* trans = new AnimFSMTransition{
        0.1f, a, b
    };
    transitions.emplace_back(trans);

    a->getTransitions().insert(trans);

    return trans;
}

AnimFSMState* AnimFSM::findAction(const std::string& name) {
    AnimFSMState* node = 0;
    for(auto& a : actions) {
        if(name == a->getName()) {
            node = a;
            break;
        }
    }
    return node;
}
int32_t AnimFSM::findActionId(const std::string& name) {
    AnimFSMState* node = 0;
    for(size_t i = 0; i < actions.size(); ++i) {
        if(name == actions[i]->getName()) {
            return i;
        }
    }
    return -1;
}

void AnimFSM::deleteAction(AnimFSMState* action) {
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
void AnimFSM::deleteTransition(AnimFSMTransition* transition) {
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

const std::vector<AnimFSMState*>&       AnimFSM::getActions() const {
    return actions;
}
const std::vector<AnimFSMTransition*>& AnimFSM::getTransitions() const {
    return transitions;
}

size_t AnimFSM::getEntryActionId() {
    return entry_action;
}
AnimFSMState* AnimFSM::getEntryAction() {
    if(entry_action < 0) return 0;
    return actions[entry_action];
}
void AnimFSM::setEntryAction(const std::string& name) {
    entry_action = findActionId(name);
    current_action = entry_action;
}

AnimFSMState* AnimFSM::getCurrentAction() {
    return actions[current_action];
}

void AnimFSM::update(
    float dt, 
    AnimSampleBuffer& sample_buffer
) {
    if(current_action > actions.size()) {
        return;
    }
    if (actions.empty()) {
        return;
    }
    auto act = actions[current_action];

    owner_motion->getBlackboard().setValue(play_count_value, (float)play_count);

    for(auto& t : act->getTransitions()) {
        bool res = false;
        for(auto& cond : t->conditions) {
            if (cond.param_hdl == -1) {
                continue;
            }
            float val = getMotion()->getBlackboard().getValue(cond.param_hdl);
            float ref_val = cond.ref_value;
            switch(cond.type) {
            case AnimFSMTransition::LARGER: res = val > ref_val; break;
            case AnimFSMTransition::LARGER_EQUAL: res = val >= ref_val; break;
            case AnimFSMTransition::LESS: res = val < ref_val; break;
            case AnimFSMTransition::LESS_EQUAL: res = val <= ref_val; break;
            case AnimFSMTransition::EQUAL: res = val == ref_val; break;
            case AnimFSMTransition::NOT_EQUAL: res = val != ref_val; break;
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
            act->setNormalCursor(.0f);
            play_count = 0;
            
            trans_samples = sample_buffer.getSamples();
            trans_speed = 1.0f / t->blendTime;
            trans_weight = .0f;
        }
    }

    
    act->update(dt, sample_buffer, getMotion()->getSkeleton().get(), trans_weight);
    owner_motion->getBlackboard().setValue(cursor_value, act->getNormalCursor());

    if(trans_weight < 1.0f) {
        for(size_t i = 0; i < sample_buffer.sampleCount(); ++i) {
            sample_buffer[i].t = gfxm::lerp(trans_samples[i].t, sample_buffer[i].t, trans_weight);
            sample_buffer[i].r = gfxm::slerp(trans_samples[i].r, sample_buffer[i].r, trans_weight);
            sample_buffer[i].s = gfxm::lerp(trans_samples[i].s, sample_buffer[i].s, trans_weight);
        }

        trans_weight += dt * trans_speed;
        if(trans_weight > 1.0f) {
            trans_weight = 1.0f;
        }
    }
}

void AnimFSM::write(out_stream& out) {
    DataWriter w(&out);

    std::map<AnimFSMState*, uint32_t> node_ids;
    std::map<AnimFSMTransition*, uint32_t> trans_ids;
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
        w.write<uint8_t>((uint8_t)a->getType());

        a->write(out);
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

    w.write(std::string());
    w.write(std::string());
}
void AnimFSM::read(in_stream& in) {
    DataReader r(&in);

    actions.resize(r.read<uint32_t>());
    transitions.resize(r.read<uint32_t>());
    for(size_t i = 0; i < transitions.size(); ++i) {
        transitions[i] = new AnimFSMTransition();
    }
    entry_action = r.read<uint32_t>();
    current_action = entry_action;

    for(size_t i = 0; i < actions.size(); ++i) {
        auto& a = actions[i];
        std::string name = r.readStr();
        gfxm::vec2 ed_pos = r.read<gfxm::vec2>();
        uint8_t motion_type = r.read<uint8_t>();
        
        if(motion_type == ANIM_FSM_STATE_CLIP) {
            a = new AnimFSMStateClip(this);
        } else if(motion_type == ANIM_FSM_STATE_FSM) {
            a = new AnimFSMStateFSM(this);
        } else if(motion_type == ANIM_FSM_STATE_BLEND_TREE) {
            a = new AnimFSMStateBlendTree(this);
        }
        if(a) {
            a->read(in);        
            a->setName(name);
            a->setEditorPos(ed_pos);
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
            cond.param_hdl = -1;
            cond.param_name = r.readStr();
            cond.type = (AnimFSMTransition::CONDITION)r.read<uint8_t>();
            cond.ref_value = r.read<float>();
            if (!cond.param_name.empty()) {
                cond.param_hdl = getMotion()->getBlackboard().getOrCreate(cond.param_name.c_str());
            }
        }
    }

    std::string ref_name = r.readStr();
    std::string skl_name = r.readStr();
}
