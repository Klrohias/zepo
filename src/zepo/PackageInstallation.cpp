//
// Created by qingy on 2024/7/15.
//

#include "PackageInstallation.hpp"

#include <ranges>

#include "Configuration.hpp"
#include "Global.hpp"
#include "NpmProtocol.hpp"
#include "async/TaskUtils.hpp"
#include "diagnostics/PerfDiagnosticsContext.hpp"
#include "network/CurlAsyncIO.hpp"
#include "semver/Range.hpp"
#include "semver/Semver.hpp"
#include "serialize/Json.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    using namespace std::string_literals;

    // Task<> PackageInstallingContext::buildDependenciesTree(const std::string& name) {
    //     // fetch package info from registry
    //     const auto url = globalConfiguration.registry + "/" + name;
    //
    //     const auto jsonDoc = JsonDocument{
    //         co_await async_io::curlEasyRequestAsync([&](CURL* instance) {
    //             curl_easy_setopt(instance, CURLOPT_URL, url.data());
    //             curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
    //         })
    //     };
    //
    //     auto npmPackageInfo = parse<NpmPackageInfo>(jsonDoc.getRootToken());
    //
    //     // find suitable version
    //     const auto& versions = npmPackageInfo.versions;
    //     const auto& versionRequirements = requirements_[name];
    //
    //     const auto result = std::find_if
    //     (
    //         versions.rbegin(),
    //         versions.rend(),
    //         [&](const std::pair<const std::string, NpmPackageVersion>& requriement) {
    //             const semver::Version currentSemver{requriement.first};
    //
    //             return std::ranges::all_of(
    //                 versionRequirements.begin(),
    //                 versionRequirements.end(),
    //                 [&](const std::string& it) {
    //                     return semver::Range{it}.satisfies(currentSemver);
    //                 });
    //         }
    //     );
    //
    //     if (result == versions.rend()) {
    //         throw std::runtime_error("Failed to find suitable version for package: \""s + name + "\"");
    //     }
    // }

    semver::Range PackageInstallingContext::getRange(const std::string_view expr) {
        // if (const auto rangeIter = versionRangeCaches_.find(expr); rangeIter != versionRangeCaches_.end()) {
        //     return rangeIter->second;
        // }
        //
        // auto parsedRange = ;
        // versionRangeCaches_.try_emplace(std::string{expr}, parsedRange);
        return semver::Range{expr};
    }

    Task<NpmPackageInfo> PackageInstallingContext::fetchPackageInfo(std::string_view packageName) {
        // fetch package info from registry
        const auto url = globalConfiguration.registry + "/" + std::string{packageName};

        ZEPO_PERF_BEGIN_(curlExecuteForPackageInfo)
        auto response = co_await async_io::curlEasyRequestAsync([&](CURL* instance) {
            curl_easy_setopt(instance, CURLOPT_URL, url.data());
            curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
        });
        ZEPO_PERF_END_(curlExecuteForPackageInfo)

        ZEPO_PERF_BEGIN_(parsePackageInfo)
        const auto jsonDoc = JsonDocument{response};
        auto result = parse<NpmPackageInfo>(jsonDoc.getRootToken());
        ZEPO_PERF_END_(parsePackageInfo)

        co_return result;
    }

    Task<> PackageInstallingContext::addRequirement(std::string_view source, std::string_view name,
                                                    std::string_view version) {
        if (version.starts_with("file:")) {
            // local file
        } else if (version.starts_with("git+") || version.starts_with("git:")) {
            // git repo
        } else if (version.starts_with("http:") || version.starts_with("https:")) {
            // internet
        } else {
            // semver
            ZEPO_PERF_BEGIN_(compileRangeExpr)
            const auto range = getRange(version);
            ZEPO_PERF_END_(compileRangeExpr)

            ZEPO_PERF_BEGIN_(fetchPackageInfo)
            const auto packageInfo = co_await fetchPackageInfo(name);
            ZEPO_PERF_END_(fetchPackageInfo)

            auto& versions = packageInfo.versions;

            ZEPO_PERF_BEGIN_(matchVersions)
            auto iter = versions.rbegin();
            for (; iter != versions.rend(); ++iter) {
                if (range.satisfies(semver::Version{iter->first}))
                    break;
            }
            ZEPO_PERF_END_(matchVersions)

            // not found
            if (iter == versions.rend()) {
                throw std::runtime_error("Failed to find suitable version for package: \"" + std::string{name} + "\"");
            }

            ZEPO_PERF_BEGIN_(pushSelectings)
            packageSelectings_.push_back({
                std::string{source},
                std::string{name},
                std::string{version},
                iter->first
            });
            ZEPO_PERF_END_(pushSelectings)

            // find depencencies
            for (auto& [nextName, nextVersion]: iter->second.dependencies) {
                co_await addRequirement(name, nextName, nextVersion);
            }
        }
    }


    Task<> PackageInstallingContext::resolveRequirements() {
        co_return;
    }
}
