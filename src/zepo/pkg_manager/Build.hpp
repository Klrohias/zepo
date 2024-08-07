//
// Created by qingy on 2024/8/7.
//


#pragma once
#ifndef ZEPO_BUILD_HPP
#define ZEPO_BUILD_HPP

#include <filesystem>
#include <string_view>
#include <optional>
#include "quickjs.h"

#include "BuildReport.hpp"
#include "zepo/Manifest.hpp"
#include "zepo/pkg_manager/BuildOptions.hpp"
#include "zepo/async/Task.hpp"
#include "zepo/semver/Range.hpp"
#include "zepo/semver/Semver.hpp"


namespace zepo::pkg_manager {
    std::optional<std::filesystem::path> resolveZepofile(
        const std::filesystem::path& packageRoot,
        const PackageManifest& manifest
    );

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        const std::filesystem::path& zepofilePath,
        const BuildOptions& options
    );

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        std::string_view packageName,
        std::string_view version,
        const BuildOptions& options
    );

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        std::string_view packageName,
        const semver::Range& version,
        const BuildOptions& options
    );
}

#endif //ZEPO_BUILD_HPP
