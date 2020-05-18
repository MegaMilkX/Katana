#ifndef DELEGATED_CALL_HPP
#define DELEGATED_CALL_HPP

#include "utils.hpp"
#include <condition_variable>

class AsyncCallWrapperBase {
public:
    virtual ~AsyncCallWrapperBase() {}
    virtual void execute() = 0;
};

void                  pushDelegatedCall(AsyncCallWrapperBase* call);
AsyncCallWrapperBase* popDelegatedCall();

template<typename RET>
class AsyncCallWrapper : public AsyncCallWrapperBase {
    std::mutex              sync;
    std::condition_variable cv;
    std::function<RET(void)> function;
    std::remove_reference<std::remove_cv<RET>> result = RET();

public:
    AsyncCallWrapper(std::function<RET(void)> fn) {
        function = fn;
    }

    void execute() override {
        result = function();
        cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(sync);
        cv.wait(lock);
    }

    RET getResult() {
        return result;
    }
};

class AsyncCallWrapperVoid : public AsyncCallWrapperBase {
    std::mutex              sync;
    std::condition_variable cv;
    std::function<void(void)> function;

public:
    AsyncCallWrapperVoid(std::function<void(void)> fn) {
        function = fn;
    }

    void execute() override {
        function();
        cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(sync);
        cv.wait(lock);
    }
};

void delegatedCall(std::function<void(void)> fn);

template<typename RET>
RET delegatedCall(std::function<RET(void)> fn) {
    if(isMainThread()) {
        return fn();
    } else {
        AsyncCallWrapper call(fn);
        pushDelegatedCall(&call);
        call.wait();
        return call.result();
    }
}

template<typename RET, typename... Args>
RET delegatedCall(RET(*fn)(Args...), Args... args) {
    delegatedCall<RET>(std::bind(fn, std::forward<Args>(args)...));
}


#endif
