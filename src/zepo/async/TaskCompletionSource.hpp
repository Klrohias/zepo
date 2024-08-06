//
// Created by NightOzZero on 2024/6/8.
//

#pragma once
#ifndef ZEPO_TASKCOMPLETIONSOURCE_HPP
#define ZEPO_TASKCOMPLETIONSOURCE_HPP

#include <memory>
#include "zepo/async/Task.hpp"

namespace zepo {
    template<typename ReturnType = void>
    class TaskCompletionSource {
        using TaskType = Task<ReturnType>;
        using AwaiterType = typename TaskType::AwaiterType;

        std::shared_ptr<AwaiterType> awaiter_{std::make_shared<AwaiterType>()};
        std::unique_ptr<TaskType> task_;

    public:
        explicit TaskCompletionSource()
            : task_{std::make_unique<TaskType>(awaiter_)} {
        }

        TaskCompletionSource(const TaskCompletionSource&) = delete;

        TaskCompletionSource(TaskCompletionSource&& other) noexcept = delete;

        [[nodiscard]] const TaskType& getTask() const {
            return *task_;
        }

        void setResult(const ReturnType& val) {
            awaiter_->setResult(new ReturnType{val});
        }

        void setResult(ReturnType&& val) {
            awaiter_->setResult(new ReturnType{std::move(val)});
        }

        void setException(const std::exception_ptr exceptionPtr) {
            awaiter_->setException(exceptionPtr);
        }
    };

    template<>
    class TaskCompletionSource<void> {
        using TaskType = Task<>;
        using AwaiterType = TaskType::AwaiterType;

        std::shared_ptr<AwaiterType> awaiter_{std::make_shared<AwaiterType>()};
        std::unique_ptr<TaskType> task_;

    public:
        explicit TaskCompletionSource()
            : task_{std::make_unique<TaskType>(awaiter_)} {
        }

        TaskCompletionSource(const TaskCompletionSource&) = delete;

        TaskCompletionSource(TaskCompletionSource&& other) noexcept = delete;

        [[nodiscard]] const TaskType& getTask() const {
            return *task_;
        }

        void setResult() {
            awaiter_->setResult(nullptr);
        }

        void setException(const std::exception_ptr& exceptionPtr) {
            awaiter_->setException(exceptionPtr);
        }
    };
}
#endif //ZEPO_TASKCOMPLETIONSOURCE_HPP
