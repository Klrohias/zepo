//
// Created by qingy on 2024/7/14.
//

#pragma once
#ifndef ZEPO_CONFIGURATION_HPP
#define ZEPO_CONFIGURATION_HPP
#include <string>
#include <optional>

#include "serialize/Reflect.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    struct Configuration {
        std::string registry{};
        std::optional<std::string> authUsername{};
        std::optional<std::string> authPassword{};
    };
}

ZEPO_REFLECT_INFO_BEGIN_(zepo::Configuration)
    ZEPO_REFLECT_FIELD_(registry);
    ZEPO_REFLECT_FIELD_(authUsername);
    ZEPO_REFLECT_FIELD_(authPassword);
ZEPO_REFLECT_INFO_END_()

ZEPO_REFLECT_PARSABLE_(zepo::Configuration);

#endif //ZEPO_CONFIGURATION_HPP
