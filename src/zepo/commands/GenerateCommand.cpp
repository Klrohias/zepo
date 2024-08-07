//
// Created by qingy on 2024/7/31.
//

#include "GenerateCommand.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <ranges>
#include <fmt/core.h>
#include <string_view>
#include <optional>

#include "quickjs-libc.h"
#include "zepo/Global.hpp"
#include "zepo/Utils.hpp"
#include "zepo/async/TaskUtils.hpp"
#include "zepo/semver/Range.hpp"
#include "zepo/js_runtime/JSEnv.hpp"
#include "zepo/js_runtime/JSUtils.hpp"
#include "zepo/pkg_manager/Build.hpp"
#include "zepo/pkg_manager/BuildOptions.hpp"

namespace zepo::commands {
    struct GenerateCmdFlags {
        std::optional<std::string_view> target;
        std::optional<std::string_view> system;
        std::filesystem::path output;
        std::optional<std::string_view> arch;
        bool dev{false};
    };

    static void showHelp() {
        fmt::println(R"(
Usage: zepo generate [build-system] [options]
  Available build systems:
    cmake
  Options:
    -A, --arch [arch], specify target architecture
    -D, --dev, includes devDependencies
    -o, --output [path], spefify the output path (directory)
    -S, --system [system], specify target system
    -T, --target [target], spefify target instead usage of -A, -S
)");
    }

    static std::string findDefaultExportName(const std::string_view packageName) {
        if (const auto indexOfPathSeparator = packageName.rfind('/');
            indexOfPathSeparator != std::string_view::npos) {
            return std::string(packageName.substr(indexOfPathSeparator + 1));
        }

        return std::string(packageName);
    }

    static std::string findExportName(const std::string_view packageName, const PackageManifest& manifest) {
        if (!manifest.zepoOptions.has_value() || !manifest.zepoOptions->packageNames.has_value()) {
            return findDefaultExportName(packageName);
        }

        auto& packageNameMap = manifest.zepoOptions->packageNames.value();
        const auto resultPair = packageNameMap.find(packageName);
        if (resultPair == packageNameMap.end()) {
            return findDefaultExportName(packageName);
        }

        return resultPair->second;
    }

    static std::map<std::string_view, std::string> findExportNames(
        const PackageManifest& manifest,
        const GenerateCmdFlags& flags
    ) {
        std::map<std::string_view, std::string> exportNames;

        for (const auto& depName: manifest.dependencies | std::views::keys) {
            exportNames.try_emplace(depName, findExportName(depName, manifest));
        }

        if (flags.dev) {
            for (const auto& depName: manifest.devDependencies | std::views::keys) {
                exportNames.try_emplace(depName, findExportName(depName, manifest));
            }
        }

        return exportNames;
    }

    static GenerateCmdFlags scanFlags(const std::span<char*>& args) {
        using namespace std::filesystem;

        // args
        auto flags = GenerateCmdFlags{};
        flags.output = current_path() / "zepo_packages";

        // scan args
        for (auto iter = args.begin(); iter != args.end(); ++iter) {
            std::string_view currentArg{*iter};
            if (!currentArg.starts_with('-')) continue;

            if (currentArg == "-o" or currentArg == "--output") {
                ++iter;
                flags.output = *iter;
            } else if (currentArg == "-D" or currentArg == "--dev") {
                flags.dev = true;
            } else if (currentArg == "-A" or currentArg == "--arch") {
                ++iter;
                flags.arch = *iter;
            } else if (currentArg == "-S" or currentArg == "--system") {
                ++iter;
                flags.system = *iter;
            } else if (currentArg == "-T" or currentArg == "--target") {
                ++iter;
                flags.target = *iter;
            }
        }

        return flags;
    }

    class CMakeGenerateContext {
        GenerateCmdFlags flags_;
        PackageManifest manifest_;
        // string_view keys always alive
        std::map<std::string_view, std::string> exportNames_{};
        pkg_manager::BuildOptions buildOptions_{};

        // js side
        JSContext* jsContext_{nullptr};
        JSValue exportNamesMap_{};

    public:
        explicit CMakeGenerateContext() = default;

        CMakeGenerateContext(const CMakeGenerateContext&) = delete;

        CMakeGenerateContext(CMakeGenerateContext&&) = delete;

        semver::Range findRequirementVersionRange(std::string_view packageName) {
            if (const auto it = manifest_.dependencies.find(packageName); it != manifest_.dependencies.end()) {
                return semver::Range{it->second};
            }

            if (const auto it = manifest_.devDependencies.find(packageName); it != manifest_.devDependencies.end()) {
                return semver::Range{it->second};
            }

            throw std::runtime_error(
                fmt::format(R"(Failed to found a version range for package "{}")", packageName));
        }

        Task<> generateCMakePackage(
            std::string_view packageName
        ) {
            using namespace std::filesystem;

            // prepare
            const auto& exportName = exportNames_[packageName];
            const auto outputPath = flags_.output / fmt::format("{}-config.cmake", exportName);

            // build
            auto buildReport = co_await buildPackage(
                jsContext_,
                packageName,
                findRequirementVersionRange(packageName),
                buildOptions_
            );

            if (!buildReport.has_value()) {
                // skip script writing
                co_return;
            }

            // write cmake script
            fmt::println(R"(Generating CMake script for package "{}" with export name "{}")", packageName, exportName);

            if (exists(outputPath)) {
                remove(outputPath);
            }

            std::fstream fileStream{outputPath, std::ios::out};
            if (!fileStream.good()) {
                throw std::runtime_error(fmt::format("Failed to open file \"{}\" for writing", outputPath.string()));
            }

            // execute generator script
            const auto moduleObject = co_await
                    js::loadESModule(jsContext_, applicationPaths.generatorsPath / "cmake.js");

            const auto generateFunction = js::getProperty(jsContext_, moduleObject, "generate");

            // call arguments
            const auto buildReportObject = js::pushCXXObject(jsContext_, buildReport.value());
            const auto packageNameString = JS_NewString(jsContext_, packageName.data());
            std::array callArguments{buildReportObject, exportNamesMap_, packageNameString};

            const auto generateResult = co_await js::awaitPromise(
                jsContext_, js::call(jsContext_, generateFunction, moduleObject, callArguments));

            fileStream << js::toString(jsContext_, generateResult);

            JS_FreeValue(jsContext_, packageNameString);
            JS_FreeValue(jsContext_, buildReportObject);
            JS_FreeValue(jsContext_, generateResult);
            JS_FreeValue(jsContext_, generateFunction);
            JS_FreeValue(jsContext_, moduleObject);
        }

        Task<> findBuildOptions() {
            if (flags_.target.has_value()) {
                const auto& targetName = flags_.target.value();
                const auto targetDefFile = applicationPaths.targetFilesPath / fmt::format("{}.js", targetName);

                if (!exists(targetDefFile)) {
                    throw std::runtime_error(fmt::format("Failed to find target {}", targetName));
                }

                const auto targetObject = co_await js::loadESModule(jsContext_, targetDefFile);

                const auto systemNameString = js::getProperty(jsContext_, targetObject, "system");
                if (systemNameString.tag == JS_TAG_STRING) {
                    buildOptions_.targetSystem = js::toString(jsContext_, systemNameString);
                }

                const auto archNameString = js::getProperty(jsContext_, targetObject, "arch");
                if (archNameString.tag == JS_TAG_STRING) {
                    buildOptions_.targetSystem = js::toString(jsContext_, archNameString);
                }

                JS_FreeValue(jsContext_, archNameString);
                JS_FreeValue(jsContext_, systemNameString);
                JS_FreeValue(jsContext_, targetObject);

                co_return;
            }

            if (flags_.arch.has_value()) {
                buildOptions_.targetArch = flags_.arch.value();
            }

            if (flags_.system.has_value()) {
                buildOptions_.targetSystem = flags_.system.value();
            }
        }

        Task<> generateCMakeDirectory(const std::span<char*>& args) {
            // prepare
            flags_ = scanFlags(args);
            manifest_ = co_await readPackageManifest();
            exportNames_ = findExportNames(manifest_, flags_);
            jsContext_ = js::initZepoJsContext(globalJsRuntime);

            co_await findBuildOptions();

            // generate and send exportNames into JSContext
            {
                JsonDocument doc;
                doc.setRoot(tokenify<JsonToken>(doc, exportNames_));
                exportNamesMap_ = js::parseJson(jsContext_, doc.stringify());
            }

            // create dir
            if (!is_directory(flags_.output)) {
                create_directories(flags_.output);
            }

            // generate
            std::vector<Task<>> tasks;

            for (const auto& packageName: manifest_.dependencies | std::views::keys) {
                tasks.emplace_back(generateCMakePackage(packageName));
            }

            if (flags_.dev) {
                for (const auto& packageName: manifest_.devDependencies | std::views::keys) {
                    tasks.emplace_back(generateCMakePackage(packageName));
                }
            }

            co_await TaskUtils::whenAll(tasks);

            // free after alloc
            JS_FreeValue(jsContext_, exportNamesMap_);
            JS_FreeContext(jsContext_);
        }
    };

    Task<> generateNpmDirectory(const std::span<char*>& args) {
        // TODO make junction links into `node_modules` directory
        co_return;
    }

    Task<> performGenerate(const std::span<char*> args) {
        if (args.empty()) {
            showHelp();
        } else if (const std::string_view buildSystem{args[0]}; buildSystem == "cmake") {
            CMakeGenerateContext context;
            co_await context.generateCMakeDirectory(args);
        } else if (buildSystem == "npm") {
            co_await generateNpmDirectory(args);
        } else {
            fmt::println("Unsupported build system \"{}\"", buildSystem);
        }

        co_return;
    }
}
