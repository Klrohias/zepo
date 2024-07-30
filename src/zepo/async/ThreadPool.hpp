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

#include "ThreadWorker.hpp"

namespace zepo
{
    class ThreadPool
    {
    public:
        using Job = ThreadWorker::Job;
    private:
        int workerCount_{0};
        int workerIdx_{0};
        std::vector<std::unique_ptr<ThreadWorker>> workers_{};

    public:
        explicit ThreadPool(int workerCount);

        static ThreadPool& getDefaultPool();

        ~ThreadPool();

        void start();

        void stop() const;

        void pool(const Job& func);

        void pool(Job&& func);
    };
} // zepo

#endif //ZEPO_THREADPOOL_HPP
