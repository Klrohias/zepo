//
// Created by qingy on 2024/7/29.
//

#include "JSEnv.hpp"
#include <quickjs-libc.h>

namespace zepo::js {
    JSRuntime* initZepoJsRuntime() {
        auto* runtime = JS_NewRuntime();
        JS_SetMemoryLimit(runtime, 1024 * 1024);
        JS_SetMaxStackSize(runtime, 1024 * 1024);

        js_std_set_worker_new_context_func(initZepoJsContext);
        js_std_init_handlers(runtime);

        JS_SetModuleLoaderFunc(runtime, nullptr, js_module_loader, nullptr);

        return runtime;
    }

    JSContext* initZepoJsContext(JSRuntime* runtime) {
        auto* ctx = JS_NewContext(runtime);
        initZepoJsContext(ctx);
        return ctx;
    }

    void initZepoJsContext(JSContext* ctx) {
        js_init_module_std(ctx, "std");
        js_init_module_os(ctx, "os");
    }
}
