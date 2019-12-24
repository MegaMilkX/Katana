#include "doc_blend_tree_2.hpp"

#include "../common/util/imgui_ext.hpp"

struct BlendTree2NodePointDesc {
    std::string name;
    rttr::type  type;
};

struct BlendTree2NodeDesc {
    std::string name;
    gfxm::vec3 gui_color;
    std::vector<BlendTree2NodePointDesc> inputs;
    std::vector<BlendTree2NodePointDesc> outputs;
};

static std::map<std::string, BlendTree2NodeDesc>& getBT2NodeDescMap() {
    static std::map<std::string, BlendTree2NodeDesc> map;
    return map;
}

class regBlendTreeNode {
    BlendTree2NodeDesc* desc_ptr;
public:
    regBlendTreeNode(const char* name) {
        desc_ptr = &getBT2NodeDescMap()[name];
        desc_ptr->name = name;
    }
    regBlendTreeNode& gui_color(float r, float g, float b) {
        desc_ptr->gui_color = gfxm::vec3(r, g, b);
        return *this;
    }
    template<typename T>
    regBlendTreeNode& in(const char* name) {
        desc_ptr->inputs.push_back({ std::string(name), rttr::type::get<T>() });
        return *this;
    }
    template<typename T>
    regBlendTreeNode& out(const char* name) {
        desc_ptr->outputs.push_back({ std::string(name), rttr::type::get<T>() });
        return *this;
    }
    regBlendTreeNode& job(void(*_job)(void)) {
        // TODO:
        return *this;
    }
};

static int foo() {
    regBlendTreeNode("AnimSample")
        .gui_color(0.6f, 0.0f, 0.2f)
        .out<int>("pose")
        .job([](){

        });
    regBlendTreeNode("Blend2")
        .gui_color(0.6f, 0.0f, 0.2f)
        .in<int>("a")
        .in<int>("b")
        .in<float>("weight")
        .out<int>("result")
        .job([](){

        });
    regBlendTreeNode("Blend3")
        .gui_color(0.6f, 0.0f, 0.2f)
        .in<int>("a")
        .in<int>("b")
        .in<int>("c")
        .in<float>("weight")
        .out<int>("result")
        .job([](){

        });
    regBlendTreeNode("Parameter")
        .gui_color(0.6f, 0.0f, 0.2f)
        .out<float>("value")
        .job([](){

        });
    return 0;
}

void DocBlendTree2::onGui(Editor* ed, float dt) {
    BlendTree2 bt;
    bt.nodes.emplace_back(BlendTree2Node{
        "Blend3", 
        gfxm::vec3(0.6f, 0.0f, 0.3f),
        gfxm::vec2(0,0),
        { 1, 2 },
        { 3 }
    });
    bt.nodes.emplace_back(BlendTree2Node{
        "AnimSample",
        gfxm::vec3(0.0f, 0.6f, 0.3f),
        gfxm::vec2(-200,-40),
        { },
        { 4 }
      });

    if(ImGuiExt::BeginGridView("BlendTree2Grid")) {
        
        for(auto& n : bt.nodes) {
            ImVec2 pos(n.gui_pos.x, n.gui_pos.y);
            ImGuiExt::BeginTreeNode(n.name.c_str(), &pos, 0, false, ImVec2(100, 100),
                ImGui::ColorConvertFloat4ToU32(ImVec4(n.gui_color.x, n.gui_color.y, n.gui_color.z, 1.0f)));

            for(auto& in : n.input_ids) {
                ImGuiExt::TreeNodeIn("test", in, 0);
            }
            for(auto& out : n.output_ids) {
                ImGuiExt::TreeNodeOut("test", out, 0);
            }

            ImGuiExt::EndTreeNode();
        }
    }
    ImGuiExt::EndGridView();
}

void DocBlendTree2::onGuiToolbox(Editor* ed) {
    
}
