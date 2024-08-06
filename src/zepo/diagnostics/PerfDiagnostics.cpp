//
// Created by qingy on 2024/7/22.
//

#include "PerfDiagnostics.hpp"

#include <fmt/core.h>

namespace zepo {
    PerfDiagnostics::PerfDiagnostics() = default;

    void PerfDiagnostics::pushTime(std::string_view kind, long timeLast) {
        std::lock_guard lockGuard{mutex_};
        if (const auto result = timeKinds_.find(kind); result != timeKinds_.end()) {
            result->second += timeLast;
        }

        timeKinds_.try_emplace(std::string{kind}, timeLast);
    }

    void PerfDiagnostics::printTimes() const {
        fmt::println("== begin print times ==");
        for (const auto& [kind, time]: timeKinds_) {
            fmt::println(" {}: {} us", kind, time);
        }

        fmt::println("== finish print times ==");
    }

    PerfDiagnostics& PerfDiagnostics::getDefault() {
        static PerfDiagnostics context{};
        return context;
    }
} // zepo
