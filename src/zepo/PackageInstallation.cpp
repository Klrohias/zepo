//
// Created by qingy on 2024/7/15.
//

#include "PackageInstallation.hpp"

#include <iostream>
#include <ranges>

#include "Configuration.hpp"
#include "NpmProtocol.hpp"
#include "async/TaskUtils.hpp"
#include "network/CurlAsyncIO.hpp"
#include "semver/Range.hpp"
#include "semver/Semver.hpp"
#include "serialize/Json.hpp"
#include "serialize/Serializer.hpp"

extern zepo::Configuration globalConfiguration;

namespace zepo {
    using namespace std::string_literals;

    Task<> PackageResolvingContext::buildDependenciesTree(const std::string& name) {
        // fetch package info from registry
        const auto url = globalConfiguration.registry + "/" + name;

        const auto jsonDoc = JsonDocument{
            co_await async_io::curlEasyRequestAsync([&](CURL* instance) {
                curl_easy_setopt(instance, CURLOPT_URL, url.data());
                curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
            })
        };

        auto npmPackageInfo = parse<NpmPackageInfo>(jsonDoc.getRootToken());

        // find suitable version
        const auto& versions = npmPackageInfo.versions;
        const auto& versionRequirements = requirements_[name];

        const auto result = std::find_if
        (
            versions.rbegin(),
            versions.rend(),
            [&](const std::pair<const std::string, NpmPackageVersion>& requriement) {
                const semver::Version currentSemver{requriement.first};

                return std::ranges::all_of(
                    versionRequirements.begin(),
                    versionRequirements.end(),
                    [&](const std::string& it) {
                        return semver::Range{it}.satisfies(currentSemver);
                    });
            }
        );

        if (result == versions.rend()) {
            throw std::runtime_error("Failed to find suitable version for package: \""s + name + "\"");
        }

        std::cout << "best version of: " << name << ": " << result->first;
    }

    void PackageResolvingContext::addRequirement(const std::string_view name, const std::string_view source) {
        if (const auto it = requirements_.find(name); it != requirements_.end()) {
            it->second.insert(std::string{source});
        } else {
            requirements_[std::string{name}] = {std::string{source}};
        }
    }

    Task<> PackageResolvingContext::resolveRequirements() {
        std::vector<Task<>> tasks{};

        for (const auto& name: requirements_ | std::views::keys) {
            tasks.push_back(buildDependenciesTree(name));
        }

        co_await TaskUtils::whenAll(tasks);
    }
}
