#include "kt_cmd.hpp"

#include "util/split.hpp"

#include "util/log.hpp"

#include <map>

static std::map<std::string, kt_cmd_callback_fn_t> callbacks;

void kt_cmd(const char* cmd) {
    std::vector<std::string> tokens = split(cmd, ' ');
    if(tokens.empty()) {
        return;
    }

    auto it = callbacks.find(tokens[0]);
    if(it == callbacks.end()) {
        LOG_WARN(tokens[0] << " - unsupported command");
        return;
    }

    std::vector<const char*> argv;
    for(auto& t : tokens) {
        argv.push_back(t.data());
    }
    it->second(tokens.size(), argv.data());
}

void kt_cmd_set_callback(const char* cmd, kt_cmd_callback_fn_t cb) {
    callbacks[cmd] = cb;
}

void kt_cmd_clear_callback(const char* cmd) {
    callbacks.erase(cmd);
}
