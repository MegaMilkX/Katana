#include <rttr/type>


class Attribute {

};

class SceneObject {
public:

};

class AttribInstance {
public:
    SceneObject* owner;
    Attribute* attrib;
};

class Scene {
public:
    SceneObject* createObject();
};

#include <iostream>
#include <string>
#include <cmath>
#include <map>

void printBinary(int v) {
    int absv = abs(v);
    if(v == 0) {
        std::cout << "0";
    } else {
        int digits = (int)log2(absv) + 1;
        if(v < 0) std::cout << "-";
        std::cout << std::noboolalpha;
        for(int i = digits - 1; i >= 0; --i) {
            std::cout << (bool)(absv & (1 << i));
        }
    }
    std::cout << std::endl;
}

void removeDups(char* pstr) {
    if(!pstr) {
        return;
    }
    size_t max_sz = strlen(pstr);
    if(max_sz < 2) {
        return;
    }

    char* pstack = new char[max_sz + 1];
    size_t stack_cur = 0;

    pstack[stack_cur] = pstr[0];
    ++stack_cur;
    for(size_t i = 1; i < max_sz; ++i) {
        if(pstr[i] != pstack[stack_cur - 1]) {
            pstack[stack_cur] = pstr[i];
            ++stack_cur;
        }
    }
    pstack[stack_cur] = '\0';

    memcpy(pstr, pstack, stack_cur + 1);

    delete[] pstack;
}

struct ListNode {
    ListNode* prev;
    ListNode* next;
    ListNode* rand;
    std::string data;
};

class List {
public:
    void clear();

    ListNode* add(const std::string& data);

    void serialize(FILE* f);
    void deserialize(FILE* f);

    void print();
private:
    ListNode* head = 0;
    ListNode* tail = 0;
    uint32_t count = 0;
};

void List::clear() {
    ListNode* cur_node = head;
    while(cur_node) {
        delete cur_node;
        cur_node = cur_node->next;
    }
    head = 0;
    tail = 0;
    count = 0;
}

ListNode* List::add(const std::string& data) {
    ListNode* new_node = new ListNode{ 0, 0, 0, data };
    if(!head) {
        head = new_node;
        tail = head;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
    count++; 
    return new_node;
}

template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
bool fwrite(FILE* f, const T* val, size_t count) {
    size_t r = fwrite((void*)val, sizeof(T), count, f);
    return r == count;
}
template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
bool fwrite(FILE* f, const T& val) {
    return fwrite(f, &val, 1);
}

void List::serialize(FILE* f) {
    std::map<ListNode*, uint32_t> ptr_to_index;
    ListNode* cur_node = head;
    uint32_t index = 1;
    while(cur_node) {
        ptr_to_index[cur_node] = index;
        ++index;
        cur_node = cur_node->next;
    }

    if(!fwrite(f, count)) {
        return;
    }

    cur_node = head;
    while(cur_node) {
        using char_t = decltype(ListNode::data)::value_type;
        size_t result = 0;
        uint64_t data_sz = cur_node->data.size();
        uint32_t rand_index = 0;

        auto it = ptr_to_index.find(cur_node->rand);
        if(it != ptr_to_index.end()) {
            rand_index = it->second;
        }

        if(!(
            fwrite(f, rand_index) &&
            fwrite(f, data_sz) &&
            fwrite(f, cur_node->data.data(), data_sz)
        )) {
            break;
        }

        cur_node = cur_node->next;
    }
}

template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
bool fread(FILE* f, T* val, size_t count) {
    size_t r = fread((void*)val, sizeof(T), count, f);
    return r == count;
}
template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
bool fread(FILE* f, T& val) {
    return fread(f, &val, 1);
}

void List::deserialize(FILE* f) {
    clear();

    if(!fread(f, count)) {
        return;
    }

    std::vector<ListNode*> nodes(count);
    for(uint32_t i = 0; i < count; ++i) {
        nodes[i] = new ListNode{ 0, 0, 0 };
    }

    bool read_err = false;
    for(uint32_t i = 0; i < count; ++i) {
        using char_t = decltype(ListNode::data)::value_type;
        uint32_t rand_index = 0;
        uint64_t data_sz = 0;
        ListNode& node = *nodes[i];

        if(!(fread(f, rand_index) &&
             fread(f, data_sz))
        ) {
            read_err = true;
            break;
        }

        node.data.resize(data_sz);
        if(!fread(f, node.data.data(), data_sz)) {
            read_err = true;
            break;
        }

        node.prev =  i > 0 ? nodes[i - 1] : 0;
        node.next = i < count - 1 ? nodes[i + 1] : 0;
        node.rand = rand_index > 0 ? nodes[rand_index - 1] : 0;
    }

    if(read_err) {
        for(size_t i = 0; i < nodes.size(); ++i) {
            delete nodes[i];
        }
        return;
    }
    
    if(!nodes.empty()) {
        head = nodes.front();
        tail = nodes.back();
    }
}

void List::print() {
    ListNode* cur_node = head;
    while(cur_node) {
        std::cout << cur_node->data << ": " << (cur_node->rand ? cur_node->rand->data : "none") << std::endl;

        cur_node = cur_node->next;
    }
}

int main() {
    char str[] = "";
    removeDups(str);
    std::cout << str << std::endl;

    printBinary(-0);
    for(int i = -32; i <= 32; ++i) {
        printBinary(i);
    }

    List list;
    FILE* f = fopen("list.bin", "wb");
    ListNode* a = list.add("test");
    a->rand = a;
    list.add("grapes");
    list.add("aaaa");
    list.add("with_rand")->rand = list.add("point_at_me");
    list.add("I_point_at_test")->rand = a;
    list.serialize(f);
    fclose(f);

    list.print();

    f = fopen("list.bin", "rb");
    list.deserialize(f);
    fclose(f);
    std::cout << "\ndeserialized: " << std::endl;
    list.print();

    std::getchar();
    return EXIT_SUCCESS;
}