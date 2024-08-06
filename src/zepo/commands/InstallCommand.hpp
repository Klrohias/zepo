//
// Created by qingy on 2024/7/31.
//


#pragma once
#ifndef ZEPO_INSTALLCOMMAND_HPP
#define ZEPO_INSTALLCOMMAND_HPP
#include <span>

#include "zepo/async/Task.hpp"


namespace zepo::commands {
    Task<> performInstall(std::span<char*> args);
}
#endif //ZEPO_INSTALLCOMMAND_HPP
