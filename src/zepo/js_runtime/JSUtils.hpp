//
// Created by qingy on 2024/7/29.
//

#pragma once
#ifndef ZEPO_ASYNC_HPP
#define ZEPO_ASYNC_HPP

#include <filesystem>
#include <quickjs.h>
#include <span>
#include <string>

#include "zepo/async/Task.hpp"

namespace zepo::js {
    Task<JSValue> awaitPromise(JSContext* ctx, JSValue value);

    JSValue eval(JSContext* ctx, std::string_view code, int flag = 0);

    JSValue call(JSContext* ctx, JSValue function, JSValue thisObj, std::span<JSValue> args);

    std::string toString(JSContext* ctx, JSValue value);

    void throwCXXException(JSContext* ctx, JSValue value);

    Task<JSValue> loadESModule(JSContext* ctx, const std::filesystem::path& filePath);

    JSValue getProperty(JSContext* ctx, JSValue value, std::string_view name);
} // namespace zepo

#endif //ZEPO_ASYNC_HPP
