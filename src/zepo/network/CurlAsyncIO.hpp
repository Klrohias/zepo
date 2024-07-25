//
// Created by qingy on 2024/7/15.
//

#pragma once
#ifndef ZEPO_CURLASYNCIO_HPP
#define ZEPO_CURLASYNCIO_HPP

#include <curl/curl.h>
#include "zepo/async/Task.hpp"
#include "zepo/async/TaskUtils.hpp"


namespace zepo::async_io {
    Task<> curlExecuteAsync(const std::function<void(CURL*)>& configAction);

    Task<> curlEasyPerformAsync(CURL* curlInstance);

    Task<> curlEasyPerformAsync(const std::shared_ptr<CURL>& curlInstance);

    Task<std::string> curlExecuteStringAsync(const std::function<void(CURL*)>& configAction);
}


#endif //ZEPO_CURLASYNCIO_HPP
