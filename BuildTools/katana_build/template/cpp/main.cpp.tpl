#include <katana.hpp>

static KatanaApi* ktApi = 0;


static class sessMain : public ktGameMode {
public:
    virtual void onStart() {
        // TODO: Initialize your session here
    }

    virtual void onUpdate() {
        // TODO: Frame update
    }

    virtual void onCleanup() {
        // TODO: Cleanup
    }
} sess_main;


int ktStartup(KatanaApi* kt_api) {
    ktApi = kt_api;
    kt_api->run(&sess_main);
    return 0;
}