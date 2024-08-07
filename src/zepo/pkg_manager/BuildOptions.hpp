//
// Created by qingy on 2024/8/6.
//


#pragma once
#ifndef ZEPO_BUILDOPTIONS_HPP
#define ZEPO_BUILDOPTIONS_HPP

#include <optional>
#include "zepo/serialize/Serializer.hpp"
#include "zepo/serialize/Reflect.hpp"

namespace zepo::pkg_manager {
    struct BuildOptions {
        std::optional<std::string> targetSystem;
        std::optional<std::string> targetArch;
    };
} // zepo::pkg_manager

ZR_BeginDef(zepo::pkg_manager::BuildOptions)
    ZR_Field(targetSystem);
    ZR_Field(targetArch);
ZR_EndDef()

ZS_MakeParsable(zepo::pkg_manager::BuildOptions);

#endif //ZEPO_BUILDOPTIONS_HPP
