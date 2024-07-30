//
// Created by qingy on 2024/7/29.
//

#pragma once
#ifndef ZEPO_JSENV_HPP
#define ZEPO_JSENV_HPP
#include <quickjs.h>

namespace zepo::js {
    JSRuntime* initZepoJsRuntime();

    JSContext* initZepoJsContext(JSRuntime* runtime);

    void initZepoJsContext(JSContext* ctx);
}

#endif //ZEPO_JSENV_HPP
