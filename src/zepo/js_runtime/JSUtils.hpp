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

#include "zepo/serialize/Json.hpp"
#include "zepo/serialize/Serializer.hpp"
#include "zepo/async/Task.hpp"

namespace zepo::js {
    Task<JSValue> awaitPromise(JSContext* ctx, JSValue value);

    Task<JSValue> tryAwaitPromise(JSContext* ctx, JSValue value);

    JSValue eval(JSContext* ctx, std::string_view code, int flag = 0);

    JSValue call(JSContext* ctx, JSValue function, JSValue thisObj, std::span<JSValue> args);

    std::string toString(JSContext* ctx, JSValue value);

    void throwCXXException(JSContext* ctx, JSValue value);

    Task<JSValue> loadESModule(JSContext* ctx, const std::filesystem::path& filePath);

    JSValue getProperty(JSContext* ctx, JSValue value, std::string_view name);

    JSValue parseJson(JSContext* ctx, std::string_view stringView, std::string_view filename = "<internal>");

    std::string stringifyJson(JSContext* ctx, JSValue value);

    template<typename Type>
    JSValue pushCXXObject(JSContext* jsContext, const Type& value) {
        JsonDocument jsonDoc{};
        jsonDoc.setRoot(tokenify<JsonToken>(jsonDoc, value));
        return parseJson(jsContext, jsonDoc.stringify());
    }

    template<typename Type>
        Type toCXXObject(JSContext* jsContext, const JSValue value) {
        const JsonDocument jsonDoc{stringifyJson(jsContext, value)};
        return parse<Type>(jsonDoc.getRootToken());
    }

    template<typename Type>
    Type toCXXObject(JSContext* jsContext, const JSValue value, JsonDocument& jsonDoc) {
        jsonDoc = JsonDocument{stringifyJson(jsContext, value)};
        return parse<Type>(jsonDoc.getRootToken());
    }
} // namespace zepo

#endif //ZEPO_ASYNC_HPP
