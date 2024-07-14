//
// Created by NightOzZero on 2024/6/8.
//

#include "ThreadPool.hpp"

#include <thread>

namespace zepo
{
    inline int detectProcessorCount()
    {
        auto detectedCount = std::thread::hardware_concurrency();
        if (!detectedCount) detectedCount = 1;
        return static_cast<int>(detectedCount);
    }

    ThreadPool ThreadPool::defaultPool_{detectProcessorCount(), true};

    ThreadPool::ThreadPool(const int workerCount, bool startImmediately): workerCount_(workerCount)
    {
        workers_.reserve(workerCount);
        if (startImmediately)
        {
            start();
        }
    }

    ThreadPool::~ThreadPool()
    {
        stop();
    }

    void ThreadPool::start()
    {
        if (started_) return;
        started_ = true;

        for (int i = 0; i < workerCount_; ++i)
        {
            workers_.emplace_back([this] { worker(); });
        }
    }

    void ThreadPool::stop()
    {
        started_ = false;
        conditionVariable_.notify_all();
        for (auto& worker : workers_)
        {
            worker.join();
        }
    }

    void ThreadPool::put(const Action& func)
    {
        std::unique_lock lock{operationLock_};
        workItems_.push(func);
        conditionVariable_.notify_one();
    }

    void ThreadPool::put(Action&& func)
    {
        std::unique_lock lock{operationLock_};
        workItems_.push(func);
        conditionVariable_.notify_one();
    }

    void ThreadPool::worker()
    {
        Action func;
        while (true)
        {
            std::this_thread::yield();

            {
                std::unique_lock lock{operationLock_};
                conditionVariable_.wait(lock, [this, &func]
                {
                    if (this->workItems_.empty()) return !started_;
                    func = std::move(workItems_.front());
                    workItems_.pop();
                    return true;
                });
            }

            if (!started_)
            {
                return;
            }

            func();
            func = nullptr;
        }
    }

    ThreadPool& ThreadPool::getDefaultPool()
    {
        return defaultPool_;
    }
} // zepo
