#ifndef SCENE_HISTORY_HPP
#define SCENE_HISTORY_HPP

#include "../common/scene/game_scene.hpp"

class SceneHistory {
public:
    struct node {
        std::string label;
        std::vector<char> data;
    };

    ~SceneHistory() {
    }

    void clear() {
        stack.clear();
        cursor = -1;
    }

    void push(GameScene* scn, const std::string& label = "") {
        if(cursor < stack.size() - 1) {
            stack.erase(stack.begin() + cursor + 1, stack.end());
        }

        dstream strm;
        scn->write(strm);
        stack.emplace_back(node{label, strm.getBuffer()});
        cursor++;
    }
    void undo(GameScene* scn) {
        if(stack.empty()) return;
        if(cursor == 0) return;
        cursor--;

        dstream strm;
        strm.setBuffer(stack[cursor].data);
        //scn->clear();
        /*
        std::vector<ktNode*> objects_to_delete;
        for(size_t i = 0; i < scn->objectCount(); ++i) {
            objects_to_delete.emplace_back(scn->getObject(i));
        }*/
        scn->read(strm);
        /*
        for(auto o : objects_to_delete) {
            scn->removeRecursive(o);
        }*/
    }
    void redo(GameScene* scn) {
        if(stack.empty()) return;
        if(stack.size() == cursor + 1) return;
        cursor++;
        
        dstream strm;
        strm.setBuffer(stack[cursor].data);
        //scn->clear();
        /*
        std::vector<ktNode*> objects_to_delete;
        for(size_t i = 0; i < scn->objectCount(); ++i) {
            objects_to_delete.emplace_back(scn->getObject(i));
        }*/
        scn->read(strm);
        /*
        for(auto o : objects_to_delete) {
            scn->removeRecursive(o);
        }*/
    }

    void onGui() {
        for(int i = stack.size() - 1; i >= 0; --i) {
            const char* pref = cursor == i ? ">" : " ";
            ImGui::Text(MKSTR(pref << " [" << i << "] " << stack[i].label).c_str());
        }
    }
private:
    int cursor = -1;
    std::vector<node> stack;
};

#endif
