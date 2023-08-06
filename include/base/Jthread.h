#pragma once
#include <thread>

namespace simpletcp::utils {

class Jthread {
public:
    template <typename FuncType, typename...Args>
    Jthread(FuncType&& func, Args&&...args)
        :mThreadHadle(std::forward<FuncType>(func), std::forward<Args>(args)...) {}

    ~Jthread() noexcept {
        if (mThreadHadle.joinable()) {
            mThreadHadle.join();
        }
    }

    // Disable copy
    Jthread(const Jthread&) = delete;
    Jthread& operator=(const Jthread&) = delete;

    // Enable move
    Jthread(Jthread&& rhs) noexcept :mThreadHadle(std::move(rhs.mThreadHadle)) {}
    Jthread& operator=(Jthread&& rhs) noexcept {
        mThreadHadle = std::move(rhs.mThreadHadle);
        return *this;
    }
private:
    std::thread mThreadHadle;
};

}
