//
// Created by qingy on 2024/7/28.
//

#pragma once
#ifndef ZEPO_INTERFACES_HPP
#define ZEPO_INTERFACES_HPP

#include "zepo/serialize/Reflect.hpp"

namespace zepo {
    struct PackageInfo {
        std::string type;
    };
}

ZEPO_REFLECT_INFO_BEGIN_(zepo::PackageInfo)
    ZEPO_REFLECT_FIELD_(type);
ZEPO_REFLECT_INFO_END_()


#endif //ZEPO_INTERFACES_HPP
