//
// Created by qingy on 2024/7/14.
//

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <curl/curl.h>

#include "Manifest.hpp"
#include "network/CurlAsyncIO.hpp"
#include "Configuration.hpp"
#include "NpmProtocol.hpp"
#include "async/AsyncIO.hpp"
#include "async/Task.hpp"
#include "serialize/Serializer.hpp"
#include "serialize/Json.hpp"
#include "PackageInstallation.hpp"
#include "async/Generator.hpp"

using namespace zepo;

Configuration globalConfiguration{};

Task<Configuration> readConfiguration(const std::string_view rootPath) {
    std::filesystem::path configPath = rootPath;
    configPath = configPath.parent_path();
    configPath /= "config.json";

    std::fstream configFile{configPath};

    const auto configContent = co_await async_io::readString(configFile);
    const JsonDocument jsonDoc{configContent};
    co_return parse<Configuration>(jsonDoc.getRootToken());
}

Task<Package> readPackageManifest() {
    const std::filesystem::path manifestPath{"package.json"};
    if (!exists(manifestPath)) {
        throw std::runtime_error("File \"package.json\" not found");
    }

    std::ifstream manifestFile{manifestPath};
    const JsonDocument jsonDoc{co_await async_io::readString(manifestFile)};
    co_return parse<Package>(jsonDoc.getRootToken());
}


Task<> performPackageResolve(const std::string& name, const std::string& source) {
    if (name.starts_with("http")) {
        // Http tarball
    } else if (name.starts_with("git")) {
        // Git
    } else if (name.starts_with("file")) {
        // Local file
    } else {
        // Semver, fetch from npm registry
        const auto url = globalConfiguration.registry + "/" + name;

        const std::string response = co_await async_io::curlEasyRequestAsync([&](auto instance) {
            curl_easy_setopt(instance, CURLOPT_URL, url.data());
            curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
        });

        const auto responseDoc = JsonDocument{response};
        auto info = parse<NpmPackageInfo>(responseDoc.getRootToken());
    }

    co_return;
}

Task<> performInstall() {
    const auto packageManifest = co_await readPackageManifest();

    PackageResolvingContext context;

    for (const auto& [packageName, source]: packageManifest.dependencies) {
        context.addRequirement(packageName, source);
    }

    for (const auto& [packageName, source]: packageManifest.devDependencies) {
        context.addRequirement(packageName, source);
    }

    co_await context.resolveRequirements();
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

        if (argc == 0) {
            shouldShowHelp = true;
        } else if (std::string_view(argv[0]) == "install") {
            co_await performInstall();
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

int main(int argc, char** argv) {
    globalConfiguration = readConfiguration(argv[0]).getValue();
    auto mainTask = asyncMain(argc, argv);
    return mainTask.getValue();
}
