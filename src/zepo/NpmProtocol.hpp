//
// Created by qingy on 2024/7/15.
//


#pragma once
#ifndef ZEPO_NPMPROTOCOL_HPP
#define ZEPO_NPMPROTOCOL_HPP
#include <filesystem>
#include <map>
#include <optional>

#include "serialize/Serializer.hpp"
#include "zepo/serialize/Reflect.hpp"
#include "zepo/async/Task.hpp"

namespace zepo {
    struct NpmPackageInfo;
    struct NpmPackageVersion;
    struct NpmPackageDist;

    struct NpmPackageInfo {
        std::string name;
        std::map<std::string, NpmPackageVersion> versions;
    };

    struct NpmPackageDist {
        std::string shasum;
        std::string tarball;
        std::string integrity;
    };

    struct NpmPackageVersion {
        std::string version;
        NpmPackageDist dist;
        std::map<std::string, std::string> dependencies;
    };

    Task<NpmPackageInfo> npmFetchMetadata(std::string_view url,
                                          std::optional<std::string_view> username,
                                          std::optional<std::string_view> password);

    Task<> npmDownloadTarball(std::string_view url,
                              std::optional<std::string_view> username,
                              std::optional<std::string_view> password,
                              std::iostream& output);

    Task<> npmDecompressArchive(const std::filesystem::path& path, const std::filesystem::path& destination);
}

ZR_BeginDef(zepo::NpmPackageInfo)
    ZR_Field(name);
    ZR_Field(versions);
ZR_EndDef()

ZS_MakeParsable(zepo::NpmPackageInfo);

ZR_BeginDef(zepo::NpmPackageVersion)
    ZR_Field(version);
    ZR_Field(dist);
    ZR_Field(dependencies);
ZR_EndDef()

ZS_MakeParsable(zepo::NpmPackageVersion);

ZR_BeginDef(zepo::NpmPackageDist)
    ZR_Field(shasum);
    ZR_Field(tarball);
    ZR_Field(integrity);
ZR_EndDef()

ZS_MakeParsable(zepo::NpmPackageDist);

#endif //ZEPO_NPMPROTOCOL_HPP
