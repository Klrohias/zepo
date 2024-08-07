//
// Created by qingy on 2024/8/7.
//


#pragma once
#ifndef ZEPO_BUILDREPORT_HPP
#define ZEPO_BUILDREPORT_HPP

#include <map>
#include <vector>
#include <string>

#include "zepo/serialize/Reflect.hpp"
#include "zepo/serialize/Serializer.hpp"
#include "zepo/serialize/Json.hpp"

namespace zepo::pkg_manager {
    struct BuildReport;
    struct OutputPathCollection;

    struct OutputPathCollection {
        std::vector<std::string> paths;

        void parse(const JsonToken& token);
    };

    struct BuildReport {
        std::map<std::string, OutputPathCollection> paths;
        std::string type;
    };
}

ZR_BeginDef(zepo::pkg_manager::BuildReport)
    ZR_Field(paths);
    ZR_Field(type);
ZR_EndDef()

ZS_MakeParsable(zepo::pkg_manager::BuildReport);

ZS_DefTokenizer(zepo::pkg_manager::OutputPathCollection, [] (DocType& doc, const auto& target) {
    return zepo::tokenify<TokenType>(doc, target.paths);
});

#endif //ZEPO_BUILDREPORT_HPP
