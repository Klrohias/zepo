//
// Created by NightOzZero on 2024/6/8.
//

#include "ThreadPool.hpp"

#include <thread>

namespace zepo {
    inline int detectProcessorCount() {
        auto detectedCount = std::thread::hardware_concurrency();
        if (!detectedCount) detectedCount = 1;
        return static_cast<int>(detectedCount);
    }

    ThreadPool::ThreadPool(const int workerCount) : workerCount_{workerCount} {
    }

    ThreadPool::~ThreadPool() {
        stop();
    }

    void ThreadPool::start() {
        for (int i = 0; i < workerCount_; ++i) {
            const auto& worker = workers_.emplace_back(
                std::make_unique<ThreadWorker>());
            worker->start();
        }
    }

    void ThreadPool::stop() const {
        for (const auto& worker: workers_) {
            worker->stop();
        }
    }

    void ThreadPool::pool(const Job& func) {
        Job copiedJob{func};

        pool(std::move(copiedJob));
    }

    void ThreadPool::pool(Job&& func) {
        for (auto& worker: workers_) {
            if (!worker->isIdle()) continue;

            worker->pool(func);
            return;
        }

        workers_[workerIdx_++]->pool(func);
        workerIdx_ = workerIdx_ % workerCount_;
    }

    ThreadPool& ThreadPool::getDefaultPool() {
        static ThreadPool defaultPool{detectProcessorCount()};
        defaultPool.start();
        return defaultPool;
    }
} // zepo
