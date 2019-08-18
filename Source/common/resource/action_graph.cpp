#include "action_graph.hpp"

#include "../lib/pugixml/pugixml.hpp"

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

void ActionGraph::pickEntryAction() {
    if(actions.empty()) {
        return;
    }
    entry_action = actions.front();
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

    if(action == entry_action) {
        pickEntryAction();
    }
}
void ActionGraph::deleteTransition(ActionGraphTransition* transition) {
    for(size_t i = 0; i < transitions.size(); ++i) {
        auto& t = transitions[i];
        if(t == transition) {
            delete t;
            transitions.erase(transitions.begin() + i);
            // TODO: Remove reference to this transition from the 'from' action
            break;
        }
    }
}

const std::vector<ActionGraphNode*>       ActionGraph::getActions() const {
    return actions;
}
const std::vector<ActionGraphTransition*> ActionGraph::getTransitions() const {
    return transitions;
}

ActionGraphNode* ActionGraph::getEntryAction() {
    return entry_action;
}
void ActionGraph::setEntryAction(const std::string& name) {
    auto a = findAction(name);
    entry_action = a;
}

void ActionGraph::serialize(out_stream& out) {
    pugi::xml_document doc;
    
}
bool ActionGraph::deserialize(in_stream& in, size_t sz) {
    return true;
}