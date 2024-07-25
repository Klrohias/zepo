//
// Created by qingy on 2024/7/15.
//

#include "PackageInstallation.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <optional>
#include <quickjs.h>

#include "Configuration.hpp"
#include "Global.hpp"
#include "NpmProtocol.hpp"
#include "async/TaskUtils.hpp"
#include "diagnostics/PerfDiagnostics.hpp"
#include "network/CurlAsyncIO.hpp"
#include "semver/Range.hpp"
#include "semver/Semver.hpp"
#include "serialize/Json.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    using namespace std::string_literals;

    const semver::Range& PackageInstallingContext::getRange(std::string_view expr) {
        if (const auto result = versionRangeCaches_.find(expr); result != versionRangeCaches_.end()) {
            return result->second;
        }

        auto range = semver::Range{expr};
        return versionRangeCaches_.try_emplace(std::string{expr}, range).first->second;
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
            ZEPO_PERF_BEGIN_(compileOrFindRangeExpr)
            const auto range = getRange(version);
            ZEPO_PERF_END_(compileOrFindRangeExpr)

            std::optional<std::string_view> authUsername;
            std::optional<std::string_view> authPassword;
            if (globalConfiguration.authUsername.has_value()) {
                authUsername = {globalConfiguration.authUsername.value()};
            }

            if (globalConfiguration.authPassword.has_value()) {
                authPassword = {globalConfiguration.authPassword.value()};
            }

            const auto packageInfo =
                    co_await npmFetchMetadata(globalConfiguration.registry + "/" + std::string{name},
                                              authUsername, authPassword);

            auto& versions = packageInfo.versions;

            ZEPO_PERF_BEGIN_(findSutiableVersion)
            auto iter = versions.rbegin();
            for (; iter != versions.rend(); ++iter) {
                if (range.satisfies(semver::Version{iter->first}))
                    break;
            }
            ZEPO_PERF_END_(findSutiableVersion)

            // not found
            if (iter == versions.rend()) {
                throw std::runtime_error("Failed to find suitable version for package: \"" + std::string{name} + "\"");
            }

            packageSelect_.push_back({
                std::string{source},
                std::string{name},
                std::string{version},
                iter->first,
                iter->second.dist.tarball
            });

            // find depencencies
            for (auto& [nextName, nextVersion]: iter->second.dependencies) {
                co_await addRequirement(name, nextName, nextVersion);
            }
        }
    }


    Task<> PackageInstallingContext::resolveRequirements() {
        ZEPO_PERF_BEGIN_(downloadPackages)

        // std::map<std::string, int>

        std::optional<std::string_view> authUsername;
        std::optional<std::string_view> authPassword;
        if (globalConfiguration.authUsername.has_value()) {
            authUsername = {globalConfiguration.authUsername.value()};
        }

        if (globalConfiguration.authPassword.has_value()) {
            authPassword = {globalConfiguration.authPassword.value()};
        }

        for (const auto& select: packageSelect_) {
            const auto outputPath = applicationPaths.downloadsPath / std::filesystem::path{select.tarball}.filename();
            if (exists(outputPath)) continue;

            const auto outputPathStr = outputPath.string();

            std::cout << "downloading: " << select.tarball << " to " << outputPathStr << std::endl;
            std::fstream outputStream{outputPath, std::fstream::out | std::fstream::binary};
            if (!outputStream.good()) {
                throw std::runtime_error("failed to open " + outputPathStr + " for package downloading");
            }

            co_await npmDownloadTarball(select.tarball, authUsername, authPassword, outputStream);
        }

        ZEPO_PERF_END_(downloadPackages)
        co_return;
    }
}
