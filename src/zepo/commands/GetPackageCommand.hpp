//
// Created by qingy on 2024/7/31.
//

#pragma once
#ifndef ZEPO_GETPACKAGECOMMAND_HPP
#define ZEPO_GETPACKAGECOMMAND_HPP
#include <span>

#include "zepo/async/Task.hpp"

namespace zepo::commands {
    Task<> performGetPackage(const std::span<char*>& args);
} // zepo::commands
#endif //ZEPO_GETPACKAGECOMMAND_HPP
