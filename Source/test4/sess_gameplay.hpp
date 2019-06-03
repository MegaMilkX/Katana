#ifndef SESS_GAMEPLAY_HPP
#define SESS_GAMEPLAY_HPP

#include "core/session.hpp"

#include <iostream>

class actorPlayerShip : public Actor {
private:
    int wpn_type = 0;
    int wpn_lv = 1;
    int speed_lv = 1;
    int missile_lv = 0;
    int option_lv = 0;
    int shield_state = 0;
};

class actorTicker : public Actor {
public:
    virtual void onStart() {
        getSession()->getScene();
    }
    virtual void onUpdate() {
        static int i = 0;
        ++i;
        std::cout << i << "\n";
    }
};

class sessGameplay : public GameSession {
    actorPlayerShip* playerShip;
public:
    virtual void onStart() {
        auto scn = getScene();
        if(!scn) return;
        
        //playerShip = (actorPlayerShip*)createActor("actorPlayerShip");
        playerShip = createActor<actorPlayerShip>();
        createActor<actorTicker>();
    }
};

#endif
