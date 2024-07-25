//
// Created by qingy on 2024/7/22.
//


#pragma once
#ifndef ZEPO_PERFDIAGNOSTICSCONTEXT_HPP
#define ZEPO_PERFDIAGNOSTICSCONTEXT_HPP

#include <mutex>
#include <chrono>
#include <map>
#include <string>

namespace zepo {
    class PerfDiagnosticsContext {
        std::map<std::string, long, std::less<>> timeKinds_{};
        std::mutex mutex_{};

    public:
        explicit PerfDiagnosticsContext();

        void pushTime(std::string_view kind, long timeLast);

        void printTimes() const;

        static PerfDiagnosticsContext& getDefault();
    };
} // zepo

#ifndef ZEPO_NO_MACROS

#define ZEPO_PERF_BEGIN_(kind) auto perf_beginOfKind##kind = std::chrono::high_resolution_clock::now();

#define ZEPO_PERF_END_(kind) auto perf_endOfKind##kind = std::chrono::high_resolution_clock::now(); \
zepo::PerfDiagnosticsContext::getDefault().pushTime(#kind, \
    std::chrono::duration_cast<std::chrono::microseconds>(perf_endOfKind##kind - perf_beginOfKind##kind).count());

#endif //ZEPO_NO_MACROS

#endif //ZEPO_PERFDIAGNOSTICSCONTEXT_HPP
