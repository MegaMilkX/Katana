#ifndef KATANA_HPP
#define KATANA_HPP

#include "session.hpp"

class KatanaApi {
public:
    virtual void run(ktGameMode*) = 0;
    virtual void stop() = 0;

    virtual float getTime() = 0;
    virtual float getDt() = 0;
};

int ktStartup(KatanaApi*);

#endif
