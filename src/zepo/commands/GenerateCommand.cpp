//
// Created by qingy on 2024/7/31.
//

#include "GenerateCommand.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <fmt/core.h>
#include <string_view>

#include "quickjs-libc.h"
#include "zepo/Global.hpp"
#include "zepo/Interfaces.hpp"
#include "zepo/Utils.hpp"
#include "zepo/async/TaskUtils.hpp"
#include "zepo/js_runtime/JSEnv.hpp"
#include "zepo/js_runtime/JSUtils.hpp"
#include "zepo/pkg_manager/PkgUtils.hpp"

namespace zepo::commands {
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

    Task<> generateCMakePackage(std::filesystem::path outputPath, std::string exportName,
                                std::string_view packageName, const PackageManifest& manifest) {
        using namespace std::filesystem;
        fmt::println(R"(Generating CMake script for package "{}" with export name "{}")", packageName, exportName);

        outputPath /= fmt::format("{}-config.cmake", exportName);

        JsonDocument doc{};
        auto packageInfo = co_await pkg_manager::buildZepoPackage(packageName, manifest, doc);

        // write cmake script
        if (exists(outputPath)) {
            remove(outputPath);
        }
        std::fstream fileStream{outputPath, std::ios::out};
        if (!fileStream.good()) {
            throw std::runtime_error(fmt::format("Failed to open file \"{}\" for writing", outputPath.string()));
        }

        doc.setRoot(tokenify<JsonToken>(doc, packageInfo));

        // execute generator script
        const auto ctx = js::initZepoJsContext(globalJsRuntime);
        const auto moduleObject = co_await js::loadESModule(ctx, applicationPaths.generatorsPath / "cmake.js");

        const auto generateFunction = js::getProperty(ctx, moduleObject, "generate");
        // args
        const auto packageInfoObjext = js::parseJson(ctx, doc.stringify());
        const auto exportNameString = JS_NewString(ctx, exportName.c_str());

        std::array args{packageInfoObjext, exportNameString};
        const auto generateResult = co_await js::awaitPromise(ctx, js::call(ctx, generateFunction, moduleObject, args));

        fileStream << js::toString(ctx, generateResult);

        JS_FreeValue(ctx, exportNameString);
        JS_FreeValue(ctx, packageInfoObjext);
        JS_FreeValue(ctx, generateResult);
        JS_FreeValue(ctx, generateFunction);
        JS_FreeValue(ctx, moduleObject);
        JS_FreeContext(ctx);
    }

    std::string getDefaultName(std::string_view packageName) {
        auto indexOfPathSeparator = packageName.rfind('/');
        if (indexOfPathSeparator != -1) {
            return std::string(packageName.substr(indexOfPathSeparator + 1));
        }

        return std::string(packageName);
    }

    Task<> generateCMakeDirectory(const std::span<char*>& args) {
        using namespace std::filesystem;

        // args
        auto outputDir = current_path() / "zepo_packages";
        auto includeDevDeps = false;

        // scan args
        for (auto iter = args.begin(); iter != args.end(); ++iter) {
            std::string_view currentArg{*iter};
            if (!currentArg.starts_with('-')) continue;

            if (currentArg == "-o" or currentArg == "--output") {
                ++iter;
                outputDir = {*iter};
            } else if (currentArg == "-D" or currentArg == "--dev") {
                includeDevDeps = true;
            }
        }

        // create dir
        if (!is_directory(outputDir)) {
            create_directories(outputDir);
        }

        // generate
        auto manifest = co_await readPackageManifest();

        std::vector<Task<>> tasks;

        // TODO: shit code, tidy it up
        for (const auto& depName: manifest.dependencies | std::views::keys) {
            const std::string exportName = [&] {
                if (!manifest.zepo.has_value() || !manifest.zepo->packageNames.has_value()) {
                    return getDefaultName(depName);
                }

                const auto& map = manifest.zepo->packageNames.value();
                if (const auto iter = map.find(depName);
                    iter != map.end()) {
                    return iter->second;
                }

                return getDefaultName(depName);
            }();

            tasks.emplace_back(generateCMakePackage(outputDir, exportName, depName, manifest));
        }
        if (includeDevDeps) {
            for (const auto& depName: manifest.devDependencies | std::views::keys) {
                const std::string exportName = [&] {
                    if (!manifest.zepo.has_value() || !manifest.zepo->packageNames.has_value()) {
                        return getDefaultName(depName);
                    }

                    const auto& map = manifest.zepo->packageNames.value();
                    if (const auto iter = map.find(depName);
                        iter != map.end()) {
                        return iter->second;
                    }

                    return getDefaultName(depName);
                }();

                tasks.emplace_back(generateCMakePackage(outputDir, exportName, depName, manifest));
            }
        }

        co_await TaskUtils::whenAll(tasks);
    }

    Task<> generateNpmDirectory(const std::span<char*>& args) {
        // TODO make junction links into `node_modules` directory
        co_return;
    }

    Task<> performGenerate(std::span<char*> args) {
        if (args.empty()) {
            showHelp();
        } else if (const std::string_view buildSystem{args[0]}; buildSystem == "cmake") {
            co_await generateCMakeDirectory(args);
        } else if (buildSystem == "npm") {
            co_await generateNpmDirectory(args);
        } else {
            fmt::println("Unsupported build system \"{}\"", buildSystem);
        }

        co_return;
    }
}
