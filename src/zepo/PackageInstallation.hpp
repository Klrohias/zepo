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
    struct NpmPackageInfo;

    class PackageInstallingContext {
        struct PackageSelect {
            std::string source;
            std::string name;
            std::string required;
            std::string selected;

            std::string tarball;
        };

        std::map<std::string, semver::Range, std::less<>> versionRangeCaches_{};
        std::vector<PackageSelect> packageSelect_{};

        const semver::Range& getRange(std::string_view expr);

    public:
        Task<> addRequirement(std::string_view source, std::string_view name, std::string_view version);

        Task<> resolveRequirements();
    };
}

#endif //ZEPO_PACKAGEINSTALLATION_HPP
