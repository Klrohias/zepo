//
// Created by qingy on 2024/7/14.
//

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <quickjs.h>
#include <fmt/core.h>

#include "Configuration.hpp"
#include "async/AsyncIO.hpp"
#include "async/Task.hpp"
#include "serialize/Serializer.hpp"
#include "serialize/Json.hpp"
#include "Global.hpp"
#include "quickjs-libc.h"
#include "js_runtime/JSEnv.hpp"
#include "js_runtime/JSUtils.hpp"
#include "commands/GenerateCommand.hpp"
#include "commands/InstallCommand.hpp"

using namespace zepo;

inline void createDirectoryIfNeed(const std::filesystem::path& path) {
    if (!is_directory(path)) {
        create_directory(path);
    }
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


static void showAppHelp() {
    fmt::println(R"(
Usage: zepo [command]
  commands:
    install, download and extract packages
    generate, generate scripts of specified build system for dependencies in package.json
    pkgconfig, let zepo act as pkgconfig (WIP)
)");
}

static Task<> performApp(const std::span<char*>& args) {
    if (args.empty()) {
        showAppHelp();
    } else if (const std::string_view command{args[0]}; command == "install") {
        co_await commands::performInstall(args.subspan(1));
    } else if (command == "generate") {
        co_await commands::performGenerate(args.subspan(1));
    } else {
        fmt::println("Unrecognized command \"{}\"", command);
        showAppHelp();
    }
}

static Task<> globalsStartup(const std::span<char*> args) {
    using namespace std::filesystem;

    if (args.size() == 0) {
        std::exit(1);
    }

    // init js runtime
    globalJsRuntime = js::initZepoJsRuntime();

    // load configuration
    globalConfiguration = co_await readConfiguration(args[0]);

    // load application paths
    path rootPath = args[0];
    rootPath = rootPath.parent_path();

    applicationPaths.packagesPath = rootPath / "packages";
    applicationPaths.downloadsPath = rootPath / "downloads";
    applicationPaths.buildsPath = rootPath / "builds";
    applicationPaths.generatorsPath = rootPath / "generators";
    applicationPaths.targetFilesPath = rootPath / "targets";

    // mkdirs
    createDirectoryIfNeed(applicationPaths.packagesPath);
    createDirectoryIfNeed(applicationPaths.downloadsPath);
    createDirectoryIfNeed(applicationPaths.buildsPath);
}

static void globalsShutdown() {
    js_std_free_handlers(globalJsRuntime);
    JS_FreeRuntime(globalJsRuntime);
}

static Task<int> asyncMain(const std::span<char*> args) {
    co_await globalsStartup(args);

    try {
        co_await performApp(args.subspan(1));
    } catch (const std::runtime_error& err) {
        fmt::println(stderr, "Error: \"{}\"", err.what());
        co_return 1;
    }

    globalsShutdown();
    co_return 0;
}

int main(int argc, char** argv) {
    using namespace std::string_view_literals;
    ThreadWorker worker;
    int exitCode{0};
    worker.pool([&] {
        const std::span args{argv, static_cast<size_t>(argc)};
        auto task = new Task{asyncMain(args)};
        task->continueWith([task, &worker, &exitCode](Task<int>::AwaiterType*) {
            exitCode = task->getValue();
            worker.stopLocal();
            delete task;
        });
    });
    worker.startLocal();
    worker.run();

    ThreadPool::getDefaultPool().stop();

    return exitCode;
}
