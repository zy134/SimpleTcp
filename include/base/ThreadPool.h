#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <list>

namespace utils {

using ThreadTaskType = std::function<void ()>;

class ThreadPool {
public:
    ThreadPool(int maxThreadNum) :mMaxThreadNum(maxThreadNum), mExited(false) {
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

    void enqueue(ThreadTaskType&& task) {
        std::lock_guard lock { mMutex };
        mTasks.emplace_back(std::move(task));
        mCond.notify_all();
    }

private:
    int                         mMaxThreadNum;
    bool                        mExited;
    std::mutex                  mMutex;
    std::condition_variable     mCond;
    std::vector<std::jthread>   mWorkers;
    std::list<ThreadTaskType>   mTasks;
};

} // namespace utils
