//
// Created by qingy on 2024/7/31.
//

#include "Interfaces.hpp"

#include <stdexcept>
#include <fmt/core.h>

namespace zepo {
    void PackagePaths::parse(const JsonToken& token) {
        const auto objectType = token.getObjectType();
        if (objectType == YYJSON_TYPE_ARR) {
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
