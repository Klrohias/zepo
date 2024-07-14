//
// Created by NightOzZero on 2024/6/8.
//

#pragma once
#ifndef ZEPO_THREADPOOL_HPP
#define ZEPO_THREADPOOL_HPP

#include <functional>
#include <future>
#include <vector>
#include <queue>

namespace zepo
{
    class ThreadPool
    {
    public:
        using Action = std::function<void()>;

    private:
        int workerCount_;
        bool started_{false};
        std::mutex operationLock_{};
        std::condition_variable conditionVariable_{};
        std::queue<Action> workItems_{};
        std::vector<std::thread> workers_{};
        static ThreadPool defaultPool_;

        void worker();

    public:
        explicit ThreadPool(int workerCount, bool startImmediately = false);

        ~ThreadPool();

        void start();

        void stop();

        void put(const Action& func);

        void put(Action&& func);

        template <typename FuncType, typename... Args>
        auto async(FuncType& func, Args... args)
        {
            using WrapperType = std::packaged_task<decltype(func(args...))()>;
            auto packagedTask = std::make_shared<WrapperType>(std::bind(func, std::forward<Args>(args)...));
            put([packagedTask] { packagedTask->operator()(); });
            return packagedTask->get_future();
        }

        static ThreadPool& getDefaultPool();
    };
} // zepo

#endif //ZEPO_THREADPOOL_HPP
