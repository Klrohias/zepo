//
// Created by NightOzZero on 2024/6/8.
//

#pragma once
#ifndef ZEPO_TASK_HPP
#define ZEPO_TASK_HPP

#include <coroutine>
#include <memory>
#include <condition_variable>
#include <functional>
#include <variant>
#include <optional>

#include "ThreadPool.hpp"

namespace zepo {
    template<typename ReturnType = void>
    class Task;

    namespace internal {
        struct BasePromise {
            BasePromise() = default;

            BasePromise(const BasePromise&) = delete;

            BasePromise(BasePromise&&) = delete;

            auto initial_suspend() noexcept {
                return std::suspend_never{};
            }

            auto final_suspend() noexcept {
                return std::suspend_always{};
            }
        };

        inline std::vector<std::coroutine_handle<>> aliveHandles_{};

        template<typename ReturnType>
        struct UnsafeAwaiter {
            using ReturnPtr = ReturnType*;
            using ContinueAction = std::function<void(UnsafeAwaiter*)>;

        private:
            std::variant<std::monostate, ReturnPtr, std::exception_ptr> value_{};
            ContinueAction continueAction_{};

            std::mutex mutex_{};
            std::condition_variable condVar_{};
            std::optional<std::coroutine_handle<>> coroHandle_;

            void notifyCompleted() {
                if (continueAction_) {
                    auto action = std::exchange(continueAction_, nullptr);
                    ThreadWorker::getCurrentWorker().pool([this, action] {
                        action(this);
                    });
                }

                condVar_.notify_all();
            }

        public:
            explicit UnsafeAwaiter() = default;

            explicit UnsafeAwaiter(std::coroutine_handle<> handle) : coroHandle_{handle} {
            }

            UnsafeAwaiter(const UnsafeAwaiter&) noexcept = delete;

            UnsafeAwaiter(UnsafeAwaiter&&) noexcept = delete;

            ~UnsafeAwaiter() {
                if constexpr (!std::is_void_v<ReturnType>) {
                    if (isSucceed()) {
                        delete getResult();
                    }
                }

                if (coroHandle_.has_value()) {
                    coroHandle_->destroy();
                }
            }

            void setResult(ReturnPtr value) {
                if (isCompleted()) {
                    throw std::runtime_error("Cannot set result twice");
                } {
                    std::unique_lock lock{mutex_};
                    value_ = {value};
                }

                notifyCompleted();
            }

            void setException(const std::exception_ptr& exceptionPtr) {
                if (isCompleted()) {
                    throw std::runtime_error("Cannot set result twice");
                } {
                    std::unique_lock lock{mutex_};
                    value_ = exceptionPtr;
                }

                notifyCompleted();
            }

            [[nodiscard]] ReturnPtr getResult() const {
                return std::get<ReturnPtr>(value_);
            }

            [[nodiscard]] std::exception_ptr getException() const {
                return std::get<std::exception_ptr>(value_);
            }

            [[nodiscard]] bool isCompleted() const {
                return !std::get_if<std::monostate>(&value_);
            }

            [[nodiscard]] bool isFailed() const {
                if (!isCompleted()) return false;
                return std::get_if<std::exception_ptr>(&value_);
            }

            [[nodiscard]] bool isSucceed() const {
                if (!isCompleted()) return false;
                return std::get_if<ReturnPtr>(&value_);
            }

            void continueWith(const ContinueAction& action) {
                bool completed{false};
                // check state
                {
                    std::unique_lock lock{mutex_};
                    completed = isCompleted();
                }

                if (completed) {
                    action(this);
                    return;
                }

                continueAction_ = action;
            }

            void wait() {
                std::unique_lock lock{mutex_};
                condVar_.wait(lock, [this] { return isCompleted(); });

                // wait for coro done
                while (coroHandle_.has_value() && !coroHandle_->done()) { std::this_thread::yield(); }
            }
        };

        template<typename ReturnType>
        struct Promise : BasePromise {
            using HandleType = std::coroutine_handle<Promise>;
            using AwaiterType = UnsafeAwaiter<ReturnType>;
            std::shared_ptr<AwaiterType> awaiter_{nullptr};

            Task<ReturnType> get_return_object();

            void return_value(ReturnType value) noexcept {
                if (awaiter_) {
                    awaiter_->setResult(new ReturnType{std::move(value)});
                    awaiter_ = nullptr;
                }
            }

            void unhandled_exception() {
                if (awaiter_) {
                    awaiter_->setException(std::current_exception());
                    awaiter_ = nullptr;
                }
            }
        };

        template<>
        struct Promise<void> : BasePromise {
            using HandleType = std::coroutine_handle<Promise>;
            using AwaiterType = UnsafeAwaiter<void>;
            std::shared_ptr<AwaiterType> awaiter_{nullptr};

            Task<> get_return_object();

            void return_void() noexcept {
                if (awaiter_) {
                    awaiter_->setResult(nullptr);
                    awaiter_ = nullptr;
                }
            }

            void unhandled_exception() {
                if (awaiter_) {
                    awaiter_->setException(std::current_exception());
                    awaiter_ = nullptr;
                }
            }
        };
    }

    // Task object
    template<typename ReturnType>
    class Task {
    public:
        using AwaiterType = internal::UnsafeAwaiter<ReturnType>;
        using PromiseType = internal::Promise<ReturnType>;

    private:
        std::shared_ptr<AwaiterType> awaiter_{};
        bool awaited_{false};

    public:
        explicit Task(const std::shared_ptr<AwaiterType>& awaiter)
            : awaiter_{awaiter} {
        }

        Task(const Task&) = default;

        Task(Task&&) = default;

        [[nodiscard]] bool await_ready() const {
            return awaiter_->isCompleted();
        }

        void await_suspend(std::coroutine_handle<> handle) {
            if (awaited_) {
                throw std::runtime_error("Task can await only once");
            }

            awaited_ = true;
            awaiter_->continueWith([handle](AwaiterType*) {
                handle.resume();
            });
        }

        ReturnType await_resume() const {
            if (awaiter_->isFailed()) {
                std::rethrow_exception(awaiter_->getException());
            }

            if constexpr (!std::is_void_v<ReturnType>) {
                return *awaiter_->getResult();
            } else {
                return;
            }
        }

        void wait() {
            awaiter_->wait();
        }

        void continueWith(const typename AwaiterType::ContinueAction& action) {
            awaiter_->continueWith(action);
        }

        ReturnType getValue() {
            wait();
            return await_resume();
        }
    };

    // internal function for creating Task object
    namespace internal {
        template<typename ReturnType>
        Task<ReturnType> Promise<ReturnType>::get_return_object() {
            if (awaiter_) {
                throw std::runtime_error("Failed to construct Task twice");
            }

            awaiter_ = std::make_shared<AwaiterType>(HandleType::from_promise(*this));
            return Task<ReturnType>{awaiter_};
        }

        inline Task<> Promise<void>::get_return_object() {
            if (awaiter_) {
                throw std::runtime_error("Failed to construct Task twice");
            }

            awaiter_ = std::make_shared<AwaiterType>(HandleType::from_promise(*this));
            return Task{awaiter_};
        }
    }
} // zepo

// traits for Task
template<typename T, typename... Args>
    requires(!std::is_reference_v<T>)
struct std::coroutine_traits<zepo::Task<T>, Args...> {
    using promise_type = typename zepo::Task<T>::PromiseType;
};

#endif //ZEPO_TASK_HPP
