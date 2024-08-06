//
// Created by qingy on 2024/7/31.
//


#pragma once
#ifndef ZEPO_UTILS_HPP
#define ZEPO_UTILS_HPP

#include <string_view>

#include "semver/Range.hpp"
#include "zepo/Manifest.hpp"
#include "zepo/async/Task.hpp"

namespace zepo {
    Task<PackageManifest> readPackageManifest(bool openMutable = false);
} // zepo

#endif //ZEPO_UTILS_HPP
