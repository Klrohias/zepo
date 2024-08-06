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

ZR_BeginDef(zepo::Configuration)
    ZR_Field(registry);
    ZR_Field(authUsername);
    ZR_Field(authPassword);
ZR_EndDef()

ZS_MakeParsable(zepo::Configuration);

#endif //ZEPO_CONFIGURATION_HPP
