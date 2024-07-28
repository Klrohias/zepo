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
#include "Global.hpp"
#include "diagnostics/PerfDiagnostics.hpp"
#include "Interfaces.hpp"
#include "quickjs-libc.h"

using namespace zepo;

inline void createDirectoryIfNeed(const std::filesystem::path& path) {
    if (!is_directory(path)) {
        create_directory(path);
    }
}

inline std::string_view findVersionRequirement(const std::string_view& packageName, const Package& manifest) {
    std::string_view versionRequirement;
    if (const auto iter = manifest.dependencies.find(packageName); iter != manifest.dependencies.end()) {
        versionRequirement = iter->second;
    } else if (const auto iter = manifest.devDependencies.find(packageName); iter != manifest.devDependencies.end()) {
        versionRequirement = iter->second;
    } else {
        throw std::runtime_error("Try to find a package which is not included in \"package.json\"");
    }

    return versionRequirement;
}

static Task<> awaitJsPromise(JSContext* ctx, JSValue value) {
    co_await TaskUtils::run<void>([&] {
        js_std_await(ctx, value);
    });
}

static Task<Configuration> readConfiguration(const std::string_view rootPath) {
    std::filesystem::path configPath = rootPath;
    configPath = configPath.parent_path();
    configPath /= "config.json";

    std::fstream configFile{configPath};

    const auto configContent = co_await async_io::readString(configFile);
    const JsonDocument jsonDoc{configContent};
    co_return parse<Configuration>(jsonDoc.getRootToken());
}

static Task<Package> readPackageManifest(bool openMutable = false) {
    const std::filesystem::path manifestPath{"package.json"};
    if (!exists(manifestPath)) {
        throw std::runtime_error("File \"package.json\" not found");
    }

    std::ifstream manifestFile{manifestPath};
    const JsonDocument jsonDoc{co_await async_io::readString(manifestFile)};
    co_return parse<Package>(jsonDoc.getRootToken());
}

static Task<> performInstall() {
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

static void showGetPackageHelp() {
    std::cout
            << "Usage: zepo get-package [package-name] [options]\n"
            << "  options: \n"
            << "    -a, --available, make the package ready for using\n"
            << std::endl;
}

static Task<> performGetPackage(const std::span<char*>& args) {
    if (args.empty()) {
        showGetPackageHelp();
        co_return;
    }

    std::string_view packageName;
    for (const auto argPtr: args) {
        std::string_view arg{argPtr};

        if (arg.starts_with('-')) {
            // options
        } else if (packageName.empty()) {
            // package name
            packageName = arg;
        } else {
            // error: multi package names
            showGetPackageHelp();
            co_return;
        }
    }

    if (packageName.empty()) {
        // error: empty package name
        showGetPackageHelp();
        co_return;
    }

    // check package
    if (!is_directory(applicationPaths.packagesPath / packageName)) {
        throw std::runtime_error("Failed to find such a package, did you forget to `zepo install` it? ");
    }

    const auto manifest = co_await readPackageManifest();
    const auto versionRequirement = findVersionRequirement(packageName, manifest);

    // find version
    semver::Range versionRange{versionRequirement};

    JSContext* ctx = JS_NewContext(globalJsRuntime);

    JS_FreeContext(ctx);
}

static void showAppHelp() {
    std::cout
            << "Usage: zepo [command]\n"
            << "  commands: \n"
            << "    install, download and extract packages\n"
            << "    get-package, get a single package's info\n"
            << std::endl;
}

static Task<> performApp(const std::span<char*>& args) {
    if (args.empty()) {
        showAppHelp();
    } else if (const std::string_view command{args[0]}; command == "install") {
        co_await performInstall();
    } else if (command == "get-package") {
        co_await performGetPackage(args.subspan(1));
    } else {
        std::cout << "Unrecognized command \""
                << command << "\"" << std::endl;

        showAppHelp();
    }
}

static void initGlobals(int argc, char** argv) {
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

static Task<int> asyncMain(const std::span<char*>& args) {
    try {
        co_await performApp(args);
    } catch (const std::runtime_error& err) {
        std::cerr << "Error: " << err.what() << std::endl;
        co_return 1;
    }

    co_return 0;
}

int main(int argc, char** argv) {
    initGlobals(argc, argv);

    // skip first argument
    {
        argc--;
        argv++;
    }

    const std::span args{argv, static_cast<size_t>(argc)};

    auto mainTask = asyncMain(args);
    const auto result = mainTask.getValue();;

    PerfDiagnostics::getDefault().printTimes();
    return result;
}
