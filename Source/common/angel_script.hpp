#ifndef ANGEL_SCRIPT_ENGINE_HPP
#define ANGEL_SCRIPT_ENGINE_HPP

#include <angelscript.h>


bool                aslInit();
void                aslCleanup();

asIScriptEngine*    aslGetEngine();


#endif
