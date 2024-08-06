//
// Created by qingy on 2024/7/28.
//

#pragma once
#ifndef ZEPO_INTERFACES_HPP
#define ZEPO_INTERFACES_HPP

#include <filesystem>
#include <map>
#include <string>

#include "zepo/serialize/Json.hpp"
#include "zepo/serialize/Serializer.hpp"
#include "zepo/serialize/Reflect.hpp"

namespace zepo {
    struct PackagePaths {
        std::vector<std::string> paths;

        void parse(const JsonToken& token);
    };

    struct PackageInfo {
        std::string type;
        std::map<std::string, PackagePaths, std::less<>> paths;
        std::map<std::string, JsonToken> extensionData;
    };
}

ZS_DefTokenizer(zepo::PackagePaths, [] (auto& doc, const TargetType& value) {
    return zepo::tokenify<TokenType>(doc, value.paths);
});

ZR_BeginDef(zepo::PackageInfo)
    ZR_Field(type);
    ZR_Field(paths);
    ZR_Attribute(SerializerExtensionDataAttribute{});
    ZR_Field(extensionData);
ZR_EndDef()

ZS_MakeParsable(zepo::PackageInfo);

ZR_MakeMetadata(zepo::PackageInfo);


#endif //ZEPO_INTERFACES_HPP
