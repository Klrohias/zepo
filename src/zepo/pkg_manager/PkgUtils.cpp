//
// Created by qingy on 2024/8/1.
//

#include "PkgUtils.hpp"

#include <fmt/core.h>
#include <quickjs-libc.h>
#include <ranges>

#include "zepo/js_runtime/JSUtils.hpp"
#include "zepo/Global.hpp"
#include "zepo/js_runtime/JSEnv.hpp"

namespace zepo::pkg_manager {
    inline std::string_view
    findVersionRequirement(const std::string_view packageName, const PackageManifest& manifest) {
        if (const auto iter = manifest.dependencies.find(packageName); iter != manifest.dependencies.end()) {
            return iter->second;
        }

        if (const auto iter = manifest.devDependencies.find(packageName); iter != manifest.devDependencies.end()) {
            return iter->second;
        }

        throw std::runtime_error("Try to find a package which is not included in \"package.json\"");
    }

    template<typename Iter>
    Iter findSuitableVersion(Iter begin,
                             Iter end,
                             const semver::Range& range,
                             std::function<semver::Version(const decltype(*begin)&)> convFunc) {
        if (auto iter = std::find_if(
            begin,
            end,
            [&](const auto& entry) {
                return range.satisfies(convFunc(entry));
            }
        ); iter != end) {
            return iter;
        }

        throw std::runtime_error("Failed to a suitable version, did you forget to `zepo install` it? ");
    }

    bool isPackageInstalled(std::string_view packageName) {
        std::filesystem::path path{};
        return isPackageInstalled(packageName, path);
    }

    bool isPackageInstalled(std::string_view packageName, std::filesystem::path& outPath) {
        outPath = applicationPaths.packagesPath / packageName;
        return is_directory(outPath);
    }

    std::filesystem::path findPackageRoot(std::string_view packageName, const semver::Range& range) {
        using namespace std::filesystem;

        // check package exists
        path packageVersionsPath{};
        if (!isPackageInstalled(packageName, packageVersionsPath)) {
            throw std::runtime_error(
                fmt::format(
                    R"(Failed to find such a package named "{}", did you forget to `zepo install` it? )",
                    packageName
                )
            );
        }

        const directory_iterator dirItorBegin{packageVersionsPath};
        const directory_iterator dirItorEnd{};
        auto packagePath = findSuitableVersion(dirItorBegin, dirItorEnd, range, [](const auto& it) {
            return semver::Version{it.path().filename().string()};
        })->path();

        return packagePath / "package";
    }

    std::filesystem::path findPackageRoot(const std::string_view packageName, const PackageManifest& manifest) {
        const semver::Range versionRange{findVersionRequirement(packageName, manifest)};
        return findPackageRoot(packageName, versionRange);
    }

    Task<JSValue> loadZepoEntry(
        const std::filesystem::path& packageRoot,
        const PackageManifest& manifest,
        JSContext* ctx
    ) {
        std::string_view entryFilename = "zepofile.js";
        if (manifest.zepoOptions.has_value() && manifest.zepoOptions->entry.has_value()) {
            entryFilename = manifest.zepoOptions.value().entry.value();
        }

        const auto entryPath = packageRoot / entryFilename;
        if (!exists(entryPath)) {
            throw std::runtime_error(
                fmt::format(
                    R"(Cannot found zepo entry "{}")",
                    entryFilename
                )
            );
        }

        // promise free automatically, do not free it
        const auto modulePromise = co_await js::loadESModule(ctx, entryPath);
        const auto moduleObject = co_await js::awaitPromise(ctx, modulePromise);

        co_return moduleObject;
    }

    inline void processPackageInfo(PackageInfo& packageInfo, const std::filesystem::path& packageRoot) {
        using namespace std::filesystem;
        for (auto& paths: packageInfo.paths | std::views::values) {
            for (auto& pathStr: paths.paths) {
                path thisPath{pathStr};
                if (thisPath.is_relative()) {
                    thisPath = packageRoot / thisPath;
                }

                pathStr = thisPath.string();
            }
        }
    }

    Task<PackageInfo> buildZepoPackage(std::string_view packageName, const PackageManifest& manifest,
        JsonDocument& doc) {
        using namespace std::filesystem;

        auto packageRoot = findPackageRoot(packageName, manifest);

        const auto ctx = js::initZepoJsContext(globalJsRuntime);

        // load module
        const auto moduleObject = co_await loadZepoEntry(packageRoot, manifest, ctx);
        const auto buildFunction = js::getProperty(ctx, moduleObject, "build");

        // build package
        const auto packageInfoPromise =
                js::call(ctx, buildFunction, moduleObject, {});
        // promise will be free after await it, don't free it manually!!

        js_std_loop(ctx);

        const auto packageInfoObject = co_await js::tryAwaitPromise(ctx, packageInfoPromise);
        doc = JsonDocument{
            js::stringifyJson(ctx, packageInfoObject),
            true
        };

        auto packageInfo = parse<PackageInfo>(doc.getRootToken());
        processPackageInfo(packageInfo, packageRoot);

        // free
        JS_FreeValue(ctx, packageInfoObject);
        JS_FreeValue(ctx, buildFunction);
        JS_FreeValue(ctx, moduleObject);
        JS_FreeContext(ctx);

        co_return packageInfo;
    }
}
