//
// Created by qingy on 2024/7/30.
//

#include "ThreadWorker.hpp"

#include <stdexcept>

namespace zepo {
    static thread_local ThreadWorker* currentWorker{nullptr};

    ThreadWorker& ThreadWorker::getCurrentWorker() {
        if (!currentWorker) {
            throw std::runtime_error("No worker in this thread");
        }

        return *currentWorker;
    }

    ThreadWorker::ThreadWorker()
        : syncGroup_{std::make_unique<SyncGroup>()} {}

    void ThreadWorker::start() {
        // check flag
        {
            std::unique_lock lock{syncGroup_->mutex_};
            if (started_) return;
            started_ = true;
        }

        // start thread
        runningThread_ = std::thread{
            [&] {
                run();
            }
        };
    }

    void ThreadWorker::startLocal() {
        started_ = true;
    }

    void ThreadWorker::run() {
        // startup
        if (currentWorker) {
            throw std::runtime_error("Thread worker conflict");
        }
        currentWorker = this;

        // loop
        while (true) {
            std::this_thread::yield();

            Job job{};

            // get next job
            {
                std::unique_lock lock{syncGroup_->mutex_};
                idle_ = true;

                syncGroup_->conditionVariable_.wait(lock, [this, &job] {
                    if (jobQueue_.empty()) return !started_;
                    job = std::move(jobQueue_.front());
                    jobQueue_.pop();
                    return true;
                });

                idle_ = false;
            }

            if (!started_) {
                break;
            }

            job();
        }

        // shutdown
        currentWorker = nullptr;
    }

    void ThreadWorker::stop() {
        // check flag
        {
            std::unique_lock lock{syncGroup_->mutex_};
            if (!started_) return;
            started_ = false;
        }

        // wait thread
        syncGroup_->conditionVariable_.notify_all();
        runningThread_.join();
    }

    void ThreadWorker::stopLocal() {
        started_ = false;
        syncGroup_->conditionVariable_.notify_all();
    }

    void ThreadWorker::pool(const Job& job) { {
            std::unique_lock lock{syncGroup_->mutex_};
            jobQueue_.push(job);
        }

        syncGroup_->conditionVariable_.notify_all();
    }

    void ThreadWorker::pool(Job&& job) { {
            std::unique_lock lock{syncGroup_->mutex_};
            jobQueue_.push(job);
        }

        syncGroup_->conditionVariable_.notify_all();
    }

    bool ThreadWorker::isIdle() const {
        std::unique_lock lock{syncGroup_->mutex_};
        return idle_;
    }
} // zepo
