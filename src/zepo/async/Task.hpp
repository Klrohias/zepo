//
// Created by NightOzZero on 2024/6/8.
//

#pragma once
#ifndef ZEPO_TASK_HPP
#define ZEPO_TASK_HPP

#include <coroutine>
#include <memory>
#include <thread>
#include <condition_variable>
#include <functional>

namespace zepo {
    template<typename ReturnType = void>
    class Task;

    namespace internal {
        struct BasePromise {
            BasePromise() = default;

            BasePromise(const BasePromise&) = delete;

            BasePromise(BasePromise&&) = delete;

            static auto initial_suspend() noexcept {
                return std::suspend_never{};
            }

            static auto final_suspend() noexcept {
                return std::suspend_always{};
            }

            static void unhandled_exception() {
                std::terminate();
            }
        };

        template<typename ReturnType>
        struct UnsafeAwaiter {
            using ContinueAction = std::function<void(UnsafeAwaiter*)>;

        private:
            ReturnType* returnValue_{nullptr};
            bool completed_{false};
            ContinueAction continueAction_{};

            std::mutex mutex_{};
            std::condition_variable condVar_{};

        public:
            explicit UnsafeAwaiter() = default;

            UnsafeAwaiter(const UnsafeAwaiter&) noexcept = delete;

            UnsafeAwaiter(UnsafeAwaiter&&) noexcept = delete;

            ~UnsafeAwaiter() {
                if constexpr (!std::is_void_v<ReturnType>) {
                    delete returnValue_;
                }
            }

            void setResult(ReturnType* value) {
                if (completed_) {
                    throw std::runtime_error("Cannot set result twice");
                }

                {
                    std::unique_lock lock{mutex_};
                    completed_ = true;
                    returnValue_ = value;
                }

                if (continueAction_) {
                    continueAction_(this);
                }

                condVar_.notify_all();
            }

            ReturnType* getResult() const {
                return returnValue_;
            }

            [[nodiscard]] bool isCompleted() const noexcept {
                return completed_;
            }

            void continueWith(const ContinueAction& action) {
                std::unique_lock lock{mutex_};

                if (completed_) {
                    action(this);
                    return;
                }

                continueAction_ = action;
            }

            void continueWith(ContinueAction&& action) {
                std::unique_lock lock{mutex_};

                if (completed_) {
                    action(this);
                    return;
                }
                continueAction_ = action;
            }

            void wait() {
                std::unique_lock lock{mutex_};
                condVar_.wait(lock, [this] { return completed_; });
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
                }
            }
        };

        template<>
        struct Promise<void> : BasePromise {
            using HandleType = std::coroutine_handle<Promise>;
            using AwaiterType = UnsafeAwaiter<void>;
            std::shared_ptr<AwaiterType> awaiter_{nullptr};

            Task<> get_return_object();

            void return_void() const noexcept {
                if (awaiter_) {
                    awaiter_->setResult(nullptr);
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
        explicit Task(std::shared_ptr<AwaiterType> awaiter)
            : awaiter_{std::move(awaiter)} {
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

            awaiter_->continueWith([handle](AwaiterType*) { handle.resume(); });
        }

        ReturnType await_resume() const {
            if constexpr (!std::is_void_v<ReturnType>) {
                return *awaiter_->getResult();
            } else {
                return;
            }
        }

        void wait() {
            awaiter_->wait();
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

            awaiter_ = std::make_shared<AwaiterType>();
            return Task<ReturnType>{awaiter_};
        }

        inline Task<> Promise<void>::get_return_object() {
            if (awaiter_) {
                throw std::runtime_error("Failed to construct Task twice");
            }

            awaiter_ = std::make_shared<AwaiterType>();
            return Task<>{awaiter_};
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
