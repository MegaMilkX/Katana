#ifndef ECS_WORLD_STORAGE_HPP
#define ECS_WORLD_STORAGE_HPP


class ecsWorldStorage {
public:
    virtual ~ecsWorldStorage() {}
    virtual void onUpdateBegin() {}
    virtual void onUpdateEnd() {}
    virtual void onGui() {}
};


#endif
