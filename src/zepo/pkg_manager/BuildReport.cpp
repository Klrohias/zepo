//
// Created by qingy on 2024/8/7.
//

#include <fmt/core.h>

#include "BuildReport.hpp"

namespace zepo::pkg_manager {
    void OutputPathCollection::parse(const JsonToken& token) {
        if (const auto objectType = token.getObjectType(); objectType == YYJSON_TYPE_ARR) {
            paths = zepo::parse<std::vector<std::string>, JsonToken>(token);
        } else if (objectType == YYJSON_TYPE_STR) {
            paths.emplace_back(zepo::parse<std::string, JsonToken>(token));
        } else if (objectType == YYJSON_TYPE_NULL) {
            // no-op
        } else {
            throw std::runtime_error(
                fmt::format("Unexpected value({}) for package paths", objectType)
            );
        }
    }
}