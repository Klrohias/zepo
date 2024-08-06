//
// Created by qingy on 2024/8/1.
//


#pragma once
#ifndef ZEPO_PKGUTILS_HPP
#define ZEPO_PKGUTILS_HPP
#include <filesystem>
#include <string_view>
#include <quickjs.h>

#include "zepo/Interfaces.hpp"
#include "zepo/Manifest.hpp"
#include "zepo/async/Task.hpp"
#include "zepo/semver/Range.hpp"

namespace zepo::pkg_manager {
    bool isPackageInstalled(std::string_view packageName);

    bool isPackageInstalled(std::string_view packageName, std::filesystem::path& outPath);

    std::filesystem::path findPackageRoot(std::string_view packageName, const semver::Range& range);

    std::filesystem::path findPackageRoot(std::string_view packageName, const PackageManifest& manifest);

    Task<JSValue> loadZepoEntry(const std::filesystem::path& packageRoot, const PackageManifest& manifest,
                                JSContext* ctx);

    Task<PackageInfo> buildZepoPackage(std::string_view packageName, const PackageManifest& manifest,
                                   JsonDocument& doc);
}

#endif //ZEPO_PKGUTILS_HPP
