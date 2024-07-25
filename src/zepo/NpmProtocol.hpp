//
// Created by qingy on 2024/7/15.
//


#pragma once
#ifndef ZEPO_NPMPROTOCOL_HPP
#define ZEPO_NPMPROTOCOL_HPP
#include <map>

#include "serialize/Reflect.hpp"

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
}

ZEPO_REFLECT_INFO_BEGIN_(zepo::NpmPackageInfo)
    ZEPO_REFLECT_FIELD_(name);
    ZEPO_REFLECT_FIELD_(versions);
ZEPO_REFLECT_INFO_END_()
ZEPO_REFLECT_PARSABLE_(zepo::NpmPackageInfo);

ZEPO_REFLECT_INFO_BEGIN_(zepo::NpmPackageVersion)
    ZEPO_REFLECT_FIELD_(version);
    ZEPO_REFLECT_FIELD_(dist);
    ZEPO_REFLECT_FIELD_(dependencies);
ZEPO_REFLECT_INFO_END_()
ZEPO_REFLECT_PARSABLE_(zepo::NpmPackageVersion);

ZEPO_REFLECT_INFO_BEGIN_(zepo::NpmPackageDist)
    ZEPO_REFLECT_FIELD_(shasum);
    ZEPO_REFLECT_FIELD_(tarball);
    ZEPO_REFLECT_FIELD_(integrity);
ZEPO_REFLECT_INFO_END_()
ZEPO_REFLECT_PARSABLE_(zepo::NpmPackageDist);

#endif //ZEPO_NPMPROTOCOL_HPP
