#ifndef ACTOR_HPP
#define ACTOR_HPP


#include <string>


class GameSession;


class Actor {
    friend GameSession;

    std::string         name            = "Actor";
    GameSession*        session         = 0;
public:
    virtual             ~Actor     () {}

    GameSession*        getSession      () { return session; }
    const std::string&  getName         () { return name; }

    virtual void        onInit          () {}
    virtual void        onUpdate        () {}
    virtual void        onCleanup       () {}
    // TODO
};

#endif
