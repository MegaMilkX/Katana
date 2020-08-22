#include "angel_script.hpp"

#include "util/log.hpp"


static asIScriptEngine* aslEngine = 0;

bool                aslInit() {
    aslEngine = asCreateScriptEngine();
    if(!aslEngine) {
        LOG_ERR("asCreateScriptEngine failed");
        return false;
    }
    return true;
}
void                aslCleanup() {
    aslEngine->ShutDownAndRelease();
}
asIScriptEngine*    aslGetEngine() {
    return aslEngine;
}