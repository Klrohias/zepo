//
// Created by qingy on 2024/7/15.
//

#pragma once
#ifndef ZEPO_PACKAGEINSTALLATION_HPP
#define ZEPO_PACKAGEINSTALLATION_HPP
#include <map>
#include <set>
#include <string>
#include <string_view>

#include "async/Task.hpp"
#include "semver/Range.hpp"

namespace zepo {
    class NpmPackageInfo;

    class PackageInstallingContext {
        struct PackageSelecting {
            std::string source;
            std::string name;
            std::string required;
            std::string selected;
        };

        std::map<std::string, semver::Range, std::less<>> versionRangeCaches_{};
        std::vector<PackageSelecting> packageSelectings_{};

        semver::Range getRange(std::string_view expr);

        static Task<NpmPackageInfo> fetchPackageInfo(std::string_view packageName);

    public:
        Task<> addRequirement(std::string_view source, std::string_view name, std::string_view version);

        Task<> resolveRequirements();
    };
}

#endif //ZEPO_PACKAGEINSTALLATION_HPP
