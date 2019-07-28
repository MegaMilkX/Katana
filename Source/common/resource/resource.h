#ifndef RESOURCE_H
#define RESOURCE_H

#include <fstream>

#include "data_source.h"
#include <rttr/type>
#include <rttr/registration>
#include <rttr/registration_friend>
#include "../util/static_run.h"
#include "../lib/imgui_wrap.hpp"

#include "../../common/util/data_stream.hpp"
#include "../../common/util/data_reader.hpp"
#include "../../common/util/data_writer.hpp"

#include "../../common/util/materialdesign_icons.hpp"

class Resource {
    RTTR_ENABLE()
public:
    enum STORAGE
    {
        GLOBAL,
        LOCAL
    };

    virtual ~Resource() {}
    const std::string& Name() const { return resource_name; }
    void Name(const std::string& name) { resource_name = name; }
    STORAGE Storage() const { return storage; }
    void Storage(STORAGE storage) { this->storage = storage; }

    virtual void serialize(out_stream& out) {}
    virtual bool deserialize(in_stream& in, size_t sz) { return false; }

    bool write_to_file(const std::string& filename) {
        dstream out;
        serialize(out);

        if(out.getBuffer().empty()) return false;

        std::ofstream f(filename, std::ios::binary);
        if(f.is_open()) {
            f.write(out.getBuffer().data(), out.getBuffer().size());
        }
        return true;
    }

    virtual const char* getWriteExtension() const { return ""; }

    virtual void _editorGui() {}
private:
    std::string resource_name;
    STORAGE storage;
};

#endif
