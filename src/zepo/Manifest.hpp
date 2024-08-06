//
// Created by qingy on 2024/7/15.
//

#pragma once
#ifndef ZEPO_MANIFEST_HPP
#define ZEPO_MANIFEST_HPP
#include <map>
#include <string>
#include <optional>

#include "serialize/Json.hpp"
#include "serialize/Reflect.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    struct PackageManifest;
    struct ZepoOptions;

    struct ZepoOptions {
        std::optional<std::map<std::string, std::string, std::less<>>> packageNames;
        std::optional<std::string> entry;
    };

    struct PackageManifest {
        std::string name;
        std::map<std::string, std::string, std::less<>> dependencies;
        std::map<std::string, std::string, std::less<>> devDependencies;
        std::string version;
        std::optional<ZepoOptions> zepoOptions;
    };
}

ZR_BeginDef(zepo::PackageManifest)
    ZR_Field(name);
    ZR_Field(dependencies);
    ZR_Field(devDependencies);
    ZR_Field(version);

    ZR_Attribute(SerializerNameAttribute{"zepo"});
    ZR_Field(zepoOptions);
ZR_EndDef()

ZS_MakeParsable(zepo::PackageManifest);

ZR_BeginDef(zepo::ZepoOptions)
    ZR_Field(packageNames);
    ZR_Field(entry);
ZR_EndDef()

ZS_MakeParsable(zepo::ZepoOptions);

#endif //ZEPO_MANIFEST_HPP
