#ifndef CONSTRAINT_STACK_HPP
#define CONSTRAINT_STACK_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "../scene/node.hpp"

#include "constraint/constraint.hpp"

class ConstraintStack : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    void update() {
        for(size_t i = 0; i < stack.size(); ++i) {
            stack[i]->update(getOwner());
        }
    }

    virtual ~ConstraintStack();
    virtual void onCreate();

    virtual void onGui() {
        for(size_t i = 0; i < stack.size(); ++i) {
            ImGui::Text(stack[i]->get_type().get_name().to_string().c_str());
            stack[i]->onGui();
            ImGui::Separator();
        }

        if(ImGui::BeginCombo("New constraint", "Select...")) {
            auto derived = rttr::type::get<Constraint::Constraint>().get_derived_classes();
            for(auto t : derived) {
                if(ImGui::Selectable(t.get_name().to_string().c_str(), false)) {
                    rttr::variant v = t.create();
                    if(!v.is_valid()) {
                        // TODO: LOG
                        continue;
                    }
                    auto cc = v.get_value<Constraint::Constraint*>();
                    if(!cc) {
                        // TODO: LOG
                        continue;
                    }
                    std::shared_ptr<Constraint::Constraint> ptr(cc);
                    //ptr->setScene(getOwner()->getScene());
                    stack.emplace_back(ptr);
                }
            }
            ImGui::EndCombo();
        }
    }

    virtual bool serialize(out_stream& out) {
        DataWriter w(&out);
        w.write<uint16_t>(stack.size());
        for(size_t i = 0; i < stack.size(); ++i) {
            w.write(stack[i]->get_type().get_name().to_string());
            stack[i]->serialize(out);
        }

        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        DataReader r(&in);
        stack.resize((size_t)r.read<uint16_t>());
        for(size_t i = 0; i < stack.size(); ++i) {
            std::string tname = r.readStr();
            rttr::type t = rttr::type::get_by_name(tname);
            if(!t.is_valid()) {
                LOG_WARN("Invalid constraint type: " << tname);
                stack.clear();
                return false;
            }
            rttr::variant v = t.create();
            if(!v.is_valid()) {
                LOG_WARN("Failed to create constraint: " << tname);
                stack.clear();
                return false;
            }
            auto p = v.get_value<Constraint::Constraint*>();
            stack[i].reset(p);
            //stack[i]->setScene(getOwner()->getScene());
            stack[i]->deserialize(in);
        }
        return true;
    }
private:
    std::vector<std::shared_ptr<Constraint::Constraint>> stack;
};

#endif
