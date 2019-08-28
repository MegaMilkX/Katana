#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

struct Node {
    std::string name;
    std::vector<std::string> ins;
    std::vector<std::string> outs;
    ImVec2 pos;
};

struct Trans {
    size_t from;
    size_t from_out;
    size_t to;
    size_t to_in;
    bool operator<(const Trans& other) const {
        return from < other.from || (!(other.from < from) && to < other.to);
    }
};

static std::vector<Node> nodes;
static std::set<Trans> transitions;

int initNodes() {
    nodes.emplace_back(
        Node{
            "test",
            { "r", "g", "b", "a" },
            { "out" },
            ImVec2(0,0)
        }
    );
    nodes.emplace_back(
        Node{
            "blend",
            { "fuck", "this", "shit", "I'm" },
            { "out" },
            ImVec2(300,0)
        }
    );
    return 0;
}

void DocBlendTree::onGui(Editor* ed, float dt) {
    static int dummy = initNodes();

    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(size_t i = 0; i < nodes.size(); ++i) {
            auto& n = nodes[i];
            ImGuiExt::BeginTreeNode(n.name.c_str(), &n.pos, ImVec2(100, 0));
            for(size_t j = 0; j < n.ins.size(); ++j) {
                auto& in = n.ins[j];
                size_t conn_node;
                size_t conn_point;
                if(ImGuiExt::TreeNodeIn(in.c_str(), &conn_node, &conn_point)) {
                    transitions.insert(Trans{ conn_node, conn_point, i, j });
                }
            }
            for(size_t j = 0; j < n.outs.size(); ++j) {
                auto& out = n.outs[j];
                size_t conn_node;
                size_t conn_point;
                if(ImGuiExt::TreeNodeOut(out.c_str(), &conn_node, &conn_point)) {
                    transitions.insert(Trans{ i, j, conn_node, conn_point });
                }
            }
            ImGuiExt::EndTreeNode();
        }
        for(auto& t : transitions) {
            ImGuiExt::TreeNodeConnection(t.from, t.to, t.from_out, t.to_in);
        }
    }
    ImGuiExt::EndGridView();
}
void DocBlendTree::onGuiToolbox(Editor* ed) {
    
}