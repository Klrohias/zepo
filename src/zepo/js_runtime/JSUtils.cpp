//
// Created by qingy on 2024/7/29.
//

#include "JSUtils.hpp"

#include <quickjs-libc.h>
#include <stdexcept>

#include "zepo/async/TaskUtils.hpp"

namespace zepo::js {
    Task<JSValue> awaitPromise(JSContext* ctx, const JSValue value) {
        co_return co_await TaskUtils::run<JSValue>([&] {
            return js_std_await(ctx, value);
        });
    }

    Task<JSValue> tryAwaitPromise(JSContext* ctx, JSValue value) {
        // check the value if is promise-like
        const auto thenAtom = JS_NewAtom(ctx, "then");
        const auto thenFunc = JS_GetProperty(ctx, value, thenAtom);
        throwCXXException(ctx, thenFunc);
        const bool isPromiseLike = thenFunc.tag != JS_TAG_UNDEFINED;
        JS_FreeValue(ctx, thenFunc);
        JS_FreeAtom(ctx, thenAtom);

        if (isPromiseLike) {
            co_return co_await awaitPromise(ctx, value);
        }

        co_return value;
    }

    JSValue eval(JSContext* ctx, std::string_view code, int flag) {
        auto result = JS_Eval(ctx, code.data(), code.size(), "<input>", flag);
        throwCXXException(ctx, result);

        return result;
    }

    JSValue call(JSContext* ctx, JSValue function, JSValue thisObj, std::span<JSValue> args) {
        auto result = JS_Call(ctx, function, thisObj, args.size(), args.data());
        throwCXXException(ctx, result);

        return result;
    }

    std::string toString(JSContext* ctx, JSValue value) {
        size_t size{0};
        auto* cstr = JS_ToCStringLen(ctx, &size, value);

        std::string result;
        result.reserve(size);
        result.append(cstr, size);

        JS_FreeCString(ctx, cstr);
        return result;
    }

    void throwCXXException(JSContext* ctx, JSValue value) {
        if (!JS_IsException(value))
            return;

        const auto exception = JS_GetException(ctx);
        throw std::runtime_error(toString(ctx, exception));
    }

    Task<JSValue> loadESModule(JSContext* ctx, const std::filesystem::path& filePath) {
        const auto parent = filePath.parent_path();
        const auto promise = JS_LoadModule(ctx, parent.string().c_str(), filePath.string().c_str());
        js_std_loop(ctx);
        co_return co_await awaitPromise(
            ctx,
            promise
        );
    }

    JSValue getProperty(JSContext* ctx, JSValue value, std::string_view name) {
        const std::string nameStr{name};

        const auto atom = JS_NewAtom(ctx, nameStr.c_str());
        const auto object = JS_GetProperty(ctx, value, atom);
        JS_FreeAtom(ctx, atom);

        throwCXXException(ctx, object);
        return object;
    }

    JSValue parseJson(JSContext* ctx, std::string_view stringView, std::string_view filename) {
        auto result = JS_ParseJSON(ctx, stringView.data(), stringView.size(), filename.data());
        throwCXXException(ctx, result);

        return result;
    }

    std::string stringifyJson(JSContext* ctx, JSValue value) {
        const auto resultString = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);
        throwCXXException(ctx, resultString);
        std::string result{toString(ctx, resultString)};
        JS_FreeValue(ctx, resultString);

        return result;
    }
} // namespace zepo
