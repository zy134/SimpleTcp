#pragma once

#include "base/Jthread.h"
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <list>
#include <future>
#include <type_traits>

namespace simpletcp {

class ThreadPool {
    using ThreadTaskType = std::function<void ()>;
public:
    ThreadPool(uint32_t maxThreadNum) :mMaxThreadNum(maxThreadNum), mExited(false) {
        if (mMaxThreadNum < 1) {
            throw std::runtime_error {"[ThreadPool] you can't initialize ThreadPool with a negative value!"};
        }

        if (mMaxThreadNum > std::thread::hardware_concurrency()) {
            mMaxThreadNum = std::thread::hardware_concurrency();
        }

        for (int i = 0; i != mMaxThreadNum; ++i) {
            mWorkers.emplace_back([&] {
                while (!mExited) {
                    ThreadTaskType task;
                    {
                        std::unique_lock lock { mMutex };
                        if (mTasks.empty()) {
                            mCond.wait(lock, [&] { return !mTasks.empty() || mExited; });
                        }
                        if (mExited) { return; }
                        task = std::move(mTasks.front());
                        mTasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() noexcept {
        mExited = true;
        // notify_all is a noexcept member function.
        mCond.notify_all();
        mWorkers.clear();
        mTasks.clear();
    }

    void exit() noexcept {
        mExited = true;
        mCond.notify_all();
    }

    template <typename FuncType, typename... Args>
    auto enqueue(FuncType&& func, Args&&...args) {
        using ResultType = std::invoke_result_t<FuncType, Args...>;
        using PackageType = std::packaged_task<ResultType()>;

        auto pPackagedTask = std::make_shared<PackageType>(
            [func = std::forward<FuncType>(func), ...args = std::forward<Args>(args)] {
                return func(args...);
            }
        );
        // std::function would copy all elements of lambda class. So we had to use std::shared_ptr
        // to save the std::packaged_task, because it can not be copied.
        auto result = pPackagedTask->get_future();
        auto task = [pPackagedTask] {
            (*pPackagedTask)();
        };

        std::lock_guard lock { mMutex };
        mTasks.emplace_back(std::move(task));
        mCond.notify_all();
        return result;
    }

private:
    uint32_t                    mMaxThreadNum;
    bool                        mExited;
    std::mutex                  mMutex;
    std::condition_variable     mCond;
    std::vector<base::jthread>  mWorkers;
    std::list<ThreadTaskType>   mTasks;
};

} // namespace utils
