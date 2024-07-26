//
// Created by qingy on 2024/7/14.
//

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <quickjs.h>

#include "Manifest.hpp"
#include "Configuration.hpp"
#include "async/AsyncIO.hpp"
#include "async/Task.hpp"
#include "serialize/Serializer.hpp"
#include "serialize/Json.hpp"
#include "PackageInstallation.hpp"
#include "async/Generator.hpp"
#include "Global.hpp"
#include "diagnostics/PerfDiagnostics.hpp"

using namespace zepo;

Task<Configuration> readConfiguration(const std::string_view rootPath) {
    std::filesystem::path configPath = rootPath;
    configPath = configPath.parent_path();
    configPath /= "config.json";

    std::fstream configFile{configPath};

    const auto configContent = co_await async_io::readString(configFile);
    const JsonDocument jsonDoc{configContent};
    co_return parse<Configuration>(jsonDoc.getRootToken());
}

Task<Package> readPackageManifest(bool openMutable = false) {
    const std::filesystem::path manifestPath{"package.json"};
    if (!exists(manifestPath)) {
        throw std::runtime_error("File \"package.json\" not found");
    }

    std::ifstream manifestFile{manifestPath};
    const JsonDocument jsonDoc{co_await async_io::readString(manifestFile)};
    co_return parse<Package>(jsonDoc.getRootToken());
}

Task<> performInstall() {
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

Task<> performGetPackage() {

}

void showHelp() {
}

Task<int> asyncMain(int argc, char** argv) {
    // skip first argument
    {
        argc--;
        argv++;
    }

    try {
        bool shouldShowHelp{false};

        const std::string_view command{argv[0]};
        if (argc == 0) {
            shouldShowHelp = true;
        } else if (command == "install") {
            co_await performInstall();
        } else if (command == "get-package") {
        } else {
            shouldShowHelp = true;
        }

        if (shouldShowHelp) {
            showHelp();
        }
    } catch (const std::runtime_error& err) {
        std::cerr << "Error: " << err.what() << std::endl;
    }

    co_return 0;
}

inline void createDirectoryIfNeed(const std::filesystem::path& path) {
    if (!is_directory(path)) {
        create_directory(path);
    }
}

void initGlobals(int argc, char** argv) {
    using namespace std::filesystem;

    if (argc == 0) {
        std::exit(1);
    }

    // init js runtime
    globalJsRuntime = JS_NewRuntime();
    JS_SetMemoryLimit(globalJsRuntime, 80 * 1024);
    JS_SetMaxStackSize(globalJsRuntime, 10 * 1024);

    // load configuration
    globalConfiguration = readConfiguration(argv[0]).getValue();

    // load application paths
    path rootPath = argv[0];
    rootPath = rootPath.parent_path();

    applicationPaths.packagesPath = rootPath / "packages";
    applicationPaths.downloadsPath = rootPath / "downloads";
    applicationPaths.buildsPath = rootPath / "builds";

    // mkdirs
    createDirectoryIfNeed(applicationPaths.packagesPath);
    createDirectoryIfNeed(applicationPaths.downloadsPath);
    createDirectoryIfNeed(applicationPaths.buildsPath);
}

int main(int argc, char** argv) {
    initGlobals(argc, argv);

    auto mainTask = asyncMain(argc, argv);
    const auto result = mainTask.getValue();;

    PerfDiagnostics::getDefault().printTimes();
    return result;
}
