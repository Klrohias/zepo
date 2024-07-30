//
// Created by qingy on 2024/7/28.
//

#pragma once
#ifndef ZEPO_INTERFACES_HPP
#define ZEPO_INTERFACES_HPP

#include <map>
#include <string>

#include "zepo/serialize/Reflect.hpp"

namespace zepo {
    struct PackageInfo {
        std::string type;
        std::map<std::string, std::string, std::less<>> paths;
    };
}

ZEPO_REFLECT_INFO_BEGIN_(zepo::PackageInfo)
    ZEPO_REFLECT_FIELD_(type);
    ZEPO_REFLECT_FIELD_(paths);
ZEPO_REFLECT_INFO_END_()
ZEPO_REFLECT_PARSABLE_(zepo::PackageInfo);


#endif //ZEPO_INTERFACES_HPP
