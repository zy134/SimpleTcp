#pragma once
#include <utility>

namespace utils {

#define DISABLE_COPY(class_name) class_name(const class_name &) = delete; \
    class_name & operator=(const class_name &) = delete;

#define DISABLE_MOVE(class_name) class_name(class_name &&) = delete; \
    class_name & operator=(class_name &&) = delete;

template <typename Func>
class ScopeGuard {
public:
    DISABLE_COPY(ScopeGuard);
    DISABLE_MOVE(ScopeGuard);

    ScopeGuard(Func&& f) :mFunc(std::move(f)), mDismiss(false) {}
    ~ScopeGuard() {
        if (!mDismiss)
            mFunc();
    }
    void dismiss() {
        mDismiss = true;
    }
private:
    Func mFunc;
    bool mDismiss;
};

} //namespace utils
