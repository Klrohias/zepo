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

namespace zepo {
    class PackageResolvingContext {
        std::map<std::string, std::set<std::string>, std::less<>> requirements_{};

        Task<> buildDependenciesTree(const std::string& name);

    public:
        void addRequirement(std::string_view name, std::string_view source);

        Task<> resolveRequirements();
    };
}

#endif //ZEPO_PACKAGEINSTALLATION_HPP
