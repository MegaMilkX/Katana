#include "delegated_call.hpp"

#include <queue>

static std::queue<AsyncCallWrapperBase*> delegated_calls;
static std::mutex                        delegated_calls_sync;

void                  pushDelegatedCall(AsyncCallWrapperBase* call) {
    std::lock_guard<std::mutex> lock(delegated_calls_sync);
    delegated_calls.push(call);
    
}
AsyncCallWrapperBase* popDelegatedCall() {
    std::lock_guard<std::mutex> lock(delegated_calls_sync);
    if (delegated_calls.empty()) {
        return 0;
    }
    auto call = delegated_calls.front();
    delegated_calls.pop();
    return call;
}


void delegatedCall(std::function<void(void)> fn) {
    if(isMainThread()) {
        fn();
    } else {
        AsyncCallWrapperVoid call(fn);
        pushDelegatedCall(&call);
        call.wait();
    }
}