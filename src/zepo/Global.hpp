//
// Created by qingy on 2024/7/21.
//

#pragma once
#ifndef ZEPO_GLOBAL_HPP
#define ZEPO_GLOBAL_HPP

#include <quickjs.h>
#include <filesystem>

#include "zepo/Configuration.hpp"

namespace zepo {
    struct ApplicationPaths {
        std::filesystem::path downloadsPath{};
        std::filesystem::path packagesPath{};
        std::filesystem::path buildsPath{};
        std::filesystem::path generatorsPath{};
    };

    extern Configuration globalConfiguration;
    extern JSRuntime* globalJsRuntime;
    extern ApplicationPaths applicationPaths;
}

#endif //ZEPO_GLOBAL_HPP
