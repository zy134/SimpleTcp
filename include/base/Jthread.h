#pragma once
#include <thread>

namespace simpletcp {

class jthread {
public:
    template <typename FuncType, typename...Args>
    jthread(FuncType&& func, Args&&...args)
        :mThreadHadle(std::forward<FuncType>(func), std::forward<Args>(args)...) {}

    ~jthread() noexcept {
        if (mThreadHadle.joinable()) {
            mThreadHadle.join();
        }
    }

    jthread(const jthread&) = delete;
    jthread(jthread&& rhs) noexcept :mThreadHadle(std::move(rhs.mThreadHadle)) {}
private:
    std::thread mThreadHadle;
};

}
