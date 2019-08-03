#ifndef ACTOR_HPP
#define ACTOR_HPP


#include <string>


class ktGameMode;


class Actor {
    friend ktGameMode;

    std::string         name            = "Actor";
    ktGameMode*          session         = 0;
public:
    virtual             ~Actor     () {}

    ktGameMode*          getSession      () { return session; }
    const std::string&  getName         () { return name; }

    virtual void        onInit          () {}
    virtual void        onUpdate        () {}
    virtual void        onCleanup       () {}
    // TODO
};

#endif
