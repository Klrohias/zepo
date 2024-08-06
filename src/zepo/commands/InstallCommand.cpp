//
// Created by qingy on 2024/7/31.
//

#include "InstallCommand.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <fmt/core.h>

#include "zepo/Global.hpp"
#include "zepo/NpmProtocol.hpp"
#include "zepo/Utils.hpp"
#include "zepo/diagnostics/PerfDiagnostics.hpp"
#include "zepo/semver/Range.hpp"

namespace zepo::commands {
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

        const semver::Range& getRange(std::string_view expr) {
            if (const auto result = versionRangeCaches_.find(expr); result != versionRangeCaches_.end()) {
                return result->second;
            }

            auto range = semver::Range{expr};
            return versionRangeCaches_.try_emplace(std::string{expr}, range).first->second;
        }

    public:
        Task<> addRequirement(std::string_view source, std::string_view name, std::string_view version) {
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
                    throw std::runtime_error(
                        "Failed to find suitable version for package: \"" + std::string{name} + "\"");
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

        Task<> resolveRequirements() {
            ZEPO_PERF_BEGIN_(downloadPackages)

            std::optional<std::string_view> authUsername;
            std::optional<std::string_view> authPassword;
            if (globalConfiguration.authUsername.has_value()) {
                authUsername = {globalConfiguration.authUsername.value()};
            }

            if (globalConfiguration.authPassword.has_value()) {
                authPassword = {globalConfiguration.authPassword.value()};
            }

            for (const auto& select: packageSelect_) {
                // download
                const auto downloadOutputPath = applicationPaths.downloadsPath / std::filesystem::path{select.tarball}.
                                                filename();
                const auto downloadOutputPathStr = downloadOutputPath.string();

                do {
                    if (exists(downloadOutputPath)) break;

                    fmt::println("downloading: {} to {}", select.tarball, downloadOutputPathStr);
                    std::fstream outputStream{downloadOutputPath, std::fstream::out | std::fstream::binary};
                    if (!outputStream.good()) {
                        throw std::runtime_error(
                            "failed to open " + downloadOutputPathStr + " for package downloading");
                    }

                    co_await npmDownloadTarball(select.tarball, authUsername, authPassword, outputStream);
                } while (false);

                // extract
                const auto extractOutputPath = applicationPaths.packagesPath / select.name / select.selected;
                do {
                    if (exists(extractOutputPath / "zepo-installation.lock")) break;
                    fmt::println("extracting: {} to {}", downloadOutputPathStr, extractOutputPath.string());
                    co_await npmDecompressArchive(downloadOutputPath, extractOutputPath);
                } while (false);
            }

            ZEPO_PERF_END_(downloadPackages)
            co_return;
        }
    };

    Task<> performInstall(std::span<char*> args) {
        const auto packageManifest = co_await readPackageManifest();

        ZEPO_PERF_BEGIN_(performInstall)
        PackageInstallingContext context;

        for (const auto& [packageName, source]: packageManifest.dependencies) {
            co_await context.addRequirement(packageManifest.name, packageName, source);
        }

        for (const auto& [packageName, source]: packageManifest.devDependencies) {
            co_await context.addRequirement(packageManifest.name, packageName, source);
        }

        co_await context.resolveRequirements();
        ZEPO_PERF_END_(performInstall)
    }
}
