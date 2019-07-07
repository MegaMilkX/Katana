#ifndef ACTOR_HPP
#define ACTOR_HPP


#include <string>


class ktSession;


class Actor {
    friend ktSession;

    std::string         name            = "Actor";
    ktSession*          session         = 0;
public:
    virtual             ~Actor     () {}

    ktSession*          getSession      () { return session; }
    const std::string&  getName         () { return name; }

    virtual void        onInit          () {}
    virtual void        onUpdate        () {}
    virtual void        onCleanup       () {}
    // TODO
};

#endif
