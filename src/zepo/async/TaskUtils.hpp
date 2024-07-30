//
// Created by NightOzZero on 2024/6/8.
//

#pragma once
#ifndef ZEPO_TASKUTILS_HPP
#define ZEPO_TASKUTILS_HPP

#include "zepo/async/Task.hpp"
#include "zepo/async/TaskCompletionSource.hpp"
#include <functional>
#include <chrono>

#include "ThreadPool.hpp"

namespace zepo
{
    class TaskUtils
    {
    public:
        template <typename ReturnType = void>
        static Task<ReturnType> run(std::function<ReturnType()> func)
        {
            auto taskCompletionSource{std::make_shared<TaskCompletionSource<ReturnType>>()};

            ThreadPool::getDefaultPool().pool([taskCompletionSource, func]
            {
                if constexpr (std::is_void_v<ReturnType>)
                {
                    func();
                    taskCompletionSource->setResult();
                }
                else
                {
                    taskCompletionSource->setResult(func());
                }
            });

            return taskCompletionSource->getTask();
        }


        template <typename T1, typename T2>
        static Task<> delay(std::chrono::duration<T1, T2> duration)
        {
            return run<void>([duration] { std::this_thread::sleep_for(duration); });
        }

        template <typename ValType, typename Iter>
        static Task<std::vector<ValType>> whenAll(Iter begin, Iter end)
        {
            std::vector<ValType> result{};
            for (auto& it = begin; it != end; ++it)
            {
                result.emplace_back(co_await *it);
            }

            co_return result;
        }

        template <typename ValType>
        static Task<std::vector<ValType>> whenAll(std::vector<Task<ValType>> tasks)
        {
            // co_return + co_await to keep the parameter `tasks` alive
            co_return co_await whenAll<ValType>(tasks.begin(), tasks.end());
        }

        template <typename Iter>
        static Task<> whenAll(Iter begin, Iter end)
        {
            for (; begin != end; ++begin)
            {
                co_await *begin;
            }
        }

        inline static Task<> whenAll(std::vector<Task<>>& tasks)
        {
            // co_await to keep the parameter `tasks` alive
            co_await whenAll(tasks.begin(), tasks.end());
        }
    };
}

#endif //ZEPO_TASKUTILS_HPP
