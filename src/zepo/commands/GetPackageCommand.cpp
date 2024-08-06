//
// Created by qingy on 2024/7/31.
//

#include "GetPackageCommand.hpp"

#include <filesystem>
#include <fmt/format.h>

#include "zepo/Interfaces.hpp"
#include "zepo/Utils.hpp"
#include "zepo/js_runtime/JSUtils.hpp"
#include "zepo/pkg_manager/PkgUtils.hpp"
#include "zepo/serialize/Json.hpp"

namespace zepo::commands {
    static void showHelp() {
        fmt::println(R"(
Usage: zepo get-package [package-name]
)");
    }

    Task<> performGetPackage(const std::span<char*>& args) {
        if (args.empty()) {
            showHelp();
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
                showHelp();
                co_return;
            }
        }

        if (packageName.empty()) {
            // error: empty package name
            showHelp();
            co_return;
        }

        // check package

        const auto manifest = co_await readPackageManifest();
        JsonDocument doc{};
        const auto result = co_await pkg_manager::buildZepoPackage(packageName, manifest, doc);
        doc.setRoot(tokenify<JsonToken>(doc, result));

        fmt::println("{}", doc.stringify());
    }
}
