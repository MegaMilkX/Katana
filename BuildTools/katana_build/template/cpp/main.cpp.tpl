#include <katana.hpp>

static KatanaApi* ktApi = 0;


static class $GAME_MODE_NAME : public ktGameMode {
    RTTR_ENABLE(ktGameMode)
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
};
STATIC_RUN($GAME_MODE_NAME) {
    rttr::registration::class_<$GAME_MODE_NAME>("$GAME_MODE_NAME")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

int ktStartup(KatanaApi* kt_api) {
    ktApi = kt_api;
    kt_api->run(new $GAME_MODE_NAME());
    return 0;
}