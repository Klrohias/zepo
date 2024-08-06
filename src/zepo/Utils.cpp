//
// Created by qingy on 2024/7/31.
//

#include "Utils.hpp"

#include <filesystem>
#include <fstream>

#include "async/AsyncIO.hpp"

namespace zepo {
    Task<PackageManifest> readPackageManifest(bool openMutable) {
        const std::filesystem::path manifestPath{"package.json"};
        if (!exists(manifestPath)) {
            throw std::runtime_error("File \"package.json\" not found");
        }

        std::ifstream manifestFile{manifestPath};
        const JsonDocument jsonDoc{co_await async_io::readString(manifestFile)};
        co_return parse<PackageManifest>(jsonDoc.getRootToken());
    }
} // zepo