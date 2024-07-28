//
// Created by qingy on 2024/7/15.
//

#pragma once
#ifndef ZEPO_MANIFEST_HPP
#define ZEPO_MANIFEST_HPP
#include <map>
#include <string>

#include "serialize/Json.hpp"
#include "serialize/Reflect.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    struct Package;

    struct Package {
        std::string name;
        std::map<std::string, std::string, std::less<>> dependencies;
        std::map<std::string, std::string, std::less<>> devDependencies;
        std::string version;
    };
}

ZEPO_REFLECT_INFO_BEGIN_(zepo::Package)
    ZEPO_REFLECT_FIELD_(name);
    ZEPO_REFLECT_FIELD_(dependencies);
    ZEPO_REFLECT_FIELD_(devDependencies);

    ZEPO_REFLECT_ATTRIBUTE_(std::string{"hello, world!"});
    ZEPO_REFLECT_FIELD_(version);
ZEPO_REFLECT_INFO_END_()

ZEPO_REFLECT_PARSABLE_(zepo::Package);

ZEPO_REFLECT_METADATA_(zepo::Package);

#endif //ZEPO_MANIFEST_HPP
