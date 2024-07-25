//
// Created by qingy on 2024/7/22.
//

#include "PerfDiagnosticsContext.hpp"

#include <iostream>

namespace zepo {
    PerfDiagnosticsContext::PerfDiagnosticsContext() = default;

    void PerfDiagnosticsContext::pushTime(std::string_view kind, long timeLast) {
        std::lock_guard lockGuard{mutex_};
        if (const auto result = timeKinds_.find(kind); result != timeKinds_.end()) {
            result->second += timeLast;
        }

        timeKinds_.try_emplace(std::string{kind}, timeLast);
    }

    void PerfDiagnosticsContext::printTimes() const {
        std::cout << "== begin print times ==" << std::endl;
        for (const auto& [kind, time]: timeKinds_) {
            std::cout << kind << ": " << time << "us\n";
        }

        std::cout << "== finish print times ==" << std::endl;
    }

    PerfDiagnosticsContext& PerfDiagnosticsContext::getDefault() {
        static PerfDiagnosticsContext context{};
        return context;
    }
} // zepo
