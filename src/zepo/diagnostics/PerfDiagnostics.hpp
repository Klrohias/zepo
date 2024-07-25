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
    class PerfDiagnostics {
        std::map<std::string, long, std::less<>> timeKinds_{};
        std::mutex mutex_{};

    public:
        explicit PerfDiagnostics();

        void pushTime(std::string_view kind, long timeLast);

        void printTimes() const;

        static PerfDiagnostics& getDefault();
    };
} // zepo

#ifndef ZEPO_NO_MACROS

#define ZEPO_PERF_BEGIN_(kind) auto perf_beginOfKind_##kind = std::chrono::high_resolution_clock::now();

#define ZEPO_PERF_END_(kind) auto perf_endOfKind_##kind = std::chrono::high_resolution_clock::now(); \
zepo::PerfDiagnostics::getDefault().pushTime(#kind, \
    std::chrono::duration_cast<std::chrono::microseconds>(perf_endOfKind_##kind - perf_beginOfKind_##kind).count());

#endif //ZEPO_NO_MACROS

#endif //ZEPO_PERFDIAGNOSTICSCONTEXT_HPP
