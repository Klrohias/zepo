//
// Created by qingy on 2024/7/17.
//

#pragma once
#ifndef ZEPO_GENERATOR_HPP
#define ZEPO_GENERATOR_HPP
#include <coroutine>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace zepo {
    template<typename YieldType>
    struct Generator;

    namespace internal {
        template<typename YieldType>
        struct GeneratorPromise {
            enum Status {
                Running = 0,
                Completed,
                Exception,
            };

        private:
            YieldType* yieldValue_{nullptr};
            std::exception_ptr exception_{};
            Status status_{Running};

        public:
            GeneratorPromise() = default;

            GeneratorPromise(const GeneratorPromise&) = delete;

            GeneratorPromise(GeneratorPromise&&) = delete;

            ~GeneratorPromise() {
                if constexpr (!std::is_void_v<YieldType>) {
                    delete yieldValue_;
                }
            }

            [[nodiscard]] Status getStatus() const {
                return status_;
            }

            [[nodiscard]] YieldType* getValue() const {
                return yieldValue_;
            }

            [[nodiscard]] const std::exception_ptr& getException() const {
                return exception_;
            }

            Generator<YieldType> get_return_object();

            auto initial_suspend() noexcept {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept {
                return std::suspend_always{};
            }

            auto yield_value(YieldType v) {
                delete yieldValue_;
                yieldValue_ = new YieldType{v};

                return std::suspend_always{};
            }

            void return_void() {
                delete yieldValue_;
                yieldValue_ = nullptr;

                status_ = Completed;
            }

            void unhandled_exception() {
                exception_ = std::current_exception();
                status_ = Exception;
            }
        };
    }

    template<typename YieldType>
    struct Generator {
        using PromiseType = internal::GeneratorPromise<YieldType>;
        using HandleType = std::coroutine_handle<PromiseType>;

    private:
        HandleType handle_{};

        void rethrowExceptionIfNeed() {
            if (auto& promise = handle_.promise(); promise.getStatus() == PromiseType::Exception) {
                std::rethrow_exception(promise.getException());
            }
        }

    public:
        explicit Generator(HandleType handle) : handle_{handle} {
        }

        Generator(const Generator&) = delete;

        Generator(Generator&&) = default;

        ~Generator() {
            if (handle_) {
                handle_.destroy();
            }
        }

        bool moveNext() {
            auto& promise = handle_.promise();
            if (promise.getStatus() == PromiseType::Completed) {
                return false;
            }

            handle_.resume();
            return promise.getStatus() != PromiseType::Completed;
        }

        YieldType& getCurrent() {
            rethrowExceptionIfNeed();
            return *handle_.promise().getValue();
        }

        [[nodiscard]] const YieldType& getCurrent() const {
            rethrowExceptionIfNeed();
            return *handle_.promise().getValue();
        }
    };

    namespace internal {
        template<typename YieldType>
        Generator<YieldType> GeneratorPromise<YieldType>::get_return_object() {
            return Generator<YieldType>{std::coroutine_handle<GeneratorPromise>::from_promise(*this)};
        }
    }
}

// traits for Task
template<typename T, typename... Args>
    requires(!std::is_reference_v<T>)
struct std::coroutine_traits<zepo::Generator<T>, Args...> {
    using promise_type = typename zepo::Generator<T>::PromiseType;
};

#endif //ZEPO_GENERATOR_HPP
