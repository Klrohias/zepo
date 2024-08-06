//
// Created by qingy on 2024/7/31.
//


#pragma once
#ifndef ZEPO_GENERATECOMMAND_HPP
#define ZEPO_GENERATECOMMAND_HPP

#include <span>

#include "zepo/async/Task.hpp"

namespace zepo::commands {
    Task<> performGenerate(std::span<char*> args);
}

#endif //ZEPO_GENERATECOMMAND_HPP
