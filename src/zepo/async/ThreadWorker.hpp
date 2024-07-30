//
// Created by qingy on 2024/7/30.
//


#pragma once
#ifndef ZEPO_THREADWORKER_HPP
#define ZEPO_THREADWORKER_HPP
#include <functional>
#include <future>
#include <thread>
#include <mutex>
#include <queue>

namespace zepo {
    class ThreadWorker {
    public:
        using Job = std::function<void()>;

    private:
        struct SyncGroup {
            std::mutex mutex_{};
            std::condition_variable conditionVariable_{};
        };

        bool started_{false};
        bool idle_{false};

        std::thread runningThread_{};
        std::unique_ptr<SyncGroup> syncGroup_{};
        std::queue<Job> jobQueue_{};

    public:
        static ThreadWorker& getCurrentWorker();

        explicit ThreadWorker();

        void start();

        void startLocal();

        void run();

        void stop();

        void stopLocal();

        void pool(const Job& job);

        void pool(Job&& job);

        [[nodiscard]] bool isIdle() const;

        template<typename FuncType, typename... Args>
        auto spawn(FuncType& func, Args... args) {
            using WrapperType = std::packaged_task<decltype(func(args...))()>;
            auto packagedTask = std::make_shared<WrapperType>(std::bind(func, std::forward<Args>(args)...));
            pool([packagedTask] { packagedTask->operator()(); });
            return packagedTask->get_future();
        }
    };
} // zepo

#endif //ZEPO_THREADWORKER_HPP
