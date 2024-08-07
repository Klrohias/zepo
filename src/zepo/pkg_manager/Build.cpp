//
// Created by qingy on 2024/8/7.
//

#include "Build.hpp"

#include <array>
#include <fmt/core.h>
#include <fstream>
#include <stdexcept>
#include <ranges>

#include "zepo/serialize/Json.hpp"
#include "zepo/serialize/Serializer.hpp"
#include "zepo/async/AsyncIO.hpp"
#include "zepo/Global.hpp"
#include "zepo/js_runtime/JSUtils.hpp"

namespace zepo::pkg_manager {
    template<typename Iter>
    Iter findSuitableVersion(
        Iter begin,
        Iter end,
        const semver::Range& range,
        std::function<semver::Version(const decltype(*begin)&)> convFunc
    ) {
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

    std::optional<std::filesystem::path> resolveZepofile(
        const std::filesystem::path& packageRoot,
        const PackageManifest& manifest
    ) {
        using namespace std::string_view_literals;

        auto defaultZepofile = "zepofile.js"sv;
        if (manifest.zepoOptions.has_value() && manifest.zepoOptions->entry.has_value()) {
            defaultZepofile = manifest.zepoOptions.value().entry.value();
        }

        auto zepofilePath = packageRoot / defaultZepofile;
        if (!exists(zepofilePath)) {
            return {};
        }

        return zepofilePath;
    }

    inline void reportToAbsolutePaths(BuildReport& buildReport, const std::filesystem::path& packageRoot) {
        using namespace std::filesystem;
        for (auto& paths: buildReport.paths | std::views::values) {
            for (auto& pathStr: paths.paths) {
                path thisPath{pathStr};
                if (thisPath.is_relative()) {
                    thisPath = packageRoot / thisPath;
                }

                pathStr = thisPath.string();
            }
        }
    }

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        const std::filesystem::path& zepofilePath,
        const BuildOptions& options
    ) {
        const auto moduleObject = co_await js::loadESModule(jsContext, zepofilePath);
        const auto buildFunction = js::getProperty(jsContext, moduleObject, "build");
        if (buildFunction.tag == JS_TAG_UNDEFINED) {
            JS_FreeValue(jsContext, moduleObject);
            co_return {};
        }

        // prepare arguments
        const auto buildOptionsObject = js::pushCXXObject(jsContext, options);
        std::array callArgument{buildOptionsObject};

        const auto buildReportObject = co_await js::tryAwaitPromise(
            jsContext,
            js::call(
                jsContext,
                buildFunction,
                JS_UNDEFINED,
                callArgument
            )
        );

        auto buildReport = js::toCXXObject<BuildReport>(jsContext, buildReportObject);

        JS_FreeValue(jsContext, buildReportObject);
        JS_FreeValue(jsContext, buildOptionsObject);
        JS_FreeValue(jsContext, buildFunction);
        JS_FreeValue(jsContext, moduleObject);

        co_return buildReport;
    }

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        std::string_view packageName,
        std::string_view version,
        const BuildOptions& options
    ) {
        // check if package exists
        const auto packageRoot = applicationPaths.packagesPath / packageName / version / "package";
        if (!is_directory(packageRoot)) {
            throw std::runtime_error(
                fmt::format(R"(failed to find such a package with packageName = "{}", version = "{}")",
                            packageName, version));
        }

        // open manifest
        std::fstream fstream{packageRoot / "package.json"};
        if (!fstream.good()) {
            throw std::runtime_error(
                fmt::format(R"(Failed to open package manifest of "{}" with version = "{}")", packageName, version));
        }
        JsonDocument jsonDoc{co_await async_io::readString(fstream)};
        fstream.close();

        auto manifest = parse<PackageManifest>(jsonDoc.getRootToken());

        // find zepofile
        auto zepofilePath = resolveZepofile(packageRoot, manifest);

        if (!zepofilePath.has_value()) {
            // skip build
            co_return {};
        }

        auto result = co_await buildPackage(jsContext, zepofilePath.value(), options);
        if (result.has_value()) {
            reportToAbsolutePaths(result.value(), packageRoot);
        }

        co_return result;
    }

    Task<std::optional<BuildReport>> buildPackage(
        JSContext* jsContext,
        std::string_view packageName,
        const semver::Range& version,
        const BuildOptions& options
    ) {
        using namespace std::filesystem;

        const auto versionsPath = applicationPaths.packagesPath / packageName;

        // find version
        directory_iterator dirItorBegin{versionsPath};
        const directory_iterator dirItorEnd{};
        std::optional<std::string> foundVersion;

        for (; dirItorBegin != dirItorEnd; ++dirItorBegin) {
            if (!dirItorBegin->is_directory()) continue;
            auto fileName = dirItorBegin->path().filename().string();
            if (version.satisfies(semver::Version{fileName})) {
                foundVersion = std::move(fileName);
                break;
            }
        }

        if (!foundVersion.has_value()) {
            throw std::runtime_error("Failed to a suitable version, did you forget to `zepo install` it? ");
        }

        co_return co_await buildPackage(jsContext, packageName, foundVersion.value(), options);
    }
}
