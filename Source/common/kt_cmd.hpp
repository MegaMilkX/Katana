#ifndef CMD_SYS_HPP
#define CMD_SYS_HPP

#include <functional>

typedef std::function<void(int argc, const char* argv[])> kt_cmd_callback_fn_t;

void kt_cmd(const char* cmd);
void kt_cmd_set_callback(const char* cmd, kt_cmd_callback_fn_t cb);
void kt_cmd_clear_callback(const char* cmd);

#endif
