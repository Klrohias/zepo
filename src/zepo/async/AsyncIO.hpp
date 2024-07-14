//
// Created by qingy on 2024/7/14.
//

#ifndef ZEPO_ASYNCIO_HPP
#define ZEPO_ASYNCIO_HPP
#include <span>

#include "Task.hpp"
#include "TaskUtils.hpp"

namespace zepo::async_io {
    template<typename StreamType>
    Task<std::streamsize> readStream(StreamType& stream, std::span<char> resultBuffer) {
        co_return co_await TaskUtils::run<std::streamsize>([&] {
            stream.read(resultBuffer.data(), resultBuffer.size());
            return stream.gcount();
        });
    }
}

#endif //ZEPO_ASYNCIO_HPP