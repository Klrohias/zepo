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
#include <optional>

#include "quickjs-libc.h"
#include "zepo/Global.hpp"
#include "zepo/Interfaces.hpp"
#include "zepo/Utils.hpp"
#include "zepo/async/TaskUtils.hpp"
#include "zepo/js_runtime/JSEnv.hpp"
#include "zepo/js_runtime/JSUtils.hpp"
#include "zepo/pkg_manager/PkgUtils.hpp"

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

    Task<> generateCMakePackage(
        JSContext* jsContext,
        const GenerateCmdFlags& flags,
        std::map<std::string_view, std::string> exportNames,
        std::string_view packageName,
        const PackageManifest& manifest
    ) {
        using namespace std::filesystem;

        const auto& exportName = exportNames[packageName];
        const auto outputPath = flags.output / fmt::format("{}-config.cmake", exportName);

        fmt::println(R"(Generating CMake script for package "{}" with export name "{}")", packageName, exportName);

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
        const auto moduleObject = co_await js::loadESModule(jsContext, applicationPaths.generatorsPath / "cmake.js");

        const auto generateFunction = js::getProperty(jsContext, moduleObject, "generate");
        // args
        const auto packageInfoObjext = js::parseJson(jsContext, doc.stringify());
        const auto exportNameString = JS_NewString(jsContext, exportName.c_str());

        std::array args{packageInfoObjext, exportNameString};
        const auto generateResult = co_await js::awaitPromise(
            jsContext, js::call(jsContext, generateFunction, moduleObject, args));

        fileStream << js::toString(jsContext, generateResult);

        JS_FreeValue(jsContext, exportNameString);
        JS_FreeValue(jsContext, packageInfoObjext);
        JS_FreeValue(jsContext, generateResult);
        JS_FreeValue(jsContext, generateFunction);
        JS_FreeValue(jsContext, moduleObject);
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

    Task<> generateCMakeDirectory(const std::span<char*>& args) {
        using namespace std::filesystem;

        // prepare
        const auto flags = scanFlags(args);
        const auto manifest = co_await readPackageManifest();
        const auto exportNames = findExportNames(manifest, flags);

        // init js context
        JSContext* jsContext = js::initZepoJsContext(globalJsRuntime);
        JSValue exportNamesArray;

        // generate and send exportNames into JSContext
        {
            JsonDocument doc;
            doc.setRoot(tokenify<JsonToken>(doc, exportNames));
            exportNamesArray = js::parseJson(jsContext, doc.stringify());
        }

        // create dir
        if (!is_directory(flags.output)) {
            create_directories(flags.output);
        }

        // generate
        std::vector<Task<>> tasks;

        co_await TaskUtils::whenAll(tasks);

        // free after alloc
        JS_FreeValue(jsContext, exportNamesArray);
        JS_FreeContext(jsContext);
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
