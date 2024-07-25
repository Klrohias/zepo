//
// Created by qingy on 2024/7/15.
//

#include "CurlAsyncIO.hpp"

#include "zepo/diagnostics/PerfDiagnosticsContext.hpp"

namespace zepo::async_io {
    static size_t curlStringWriter(void* buffer, size_t size, size_t count, void* result) {
        ZEPO_PERF_BEGIN_(curlAppend)
        static_cast<std::string*>(result)->append(static_cast<char*>(buffer), size * count);
        ZEPO_PERF_END_(curlAppend)

        return size * count;
    }

    Task<> async_io::curlEasyPerformAsync(CURL* curlInstance) {
        co_await TaskUtils::run<void>([curlInstance] {
            curl_easy_perform(curlInstance);
        });
    }

    Task<> curlEasyPerformAsync(const std::shared_ptr<CURL>& curlInstance) {
        co_await curlEasyPerformAsync(curlInstance.get());
    }

    Task<std::string> curlEasyRequestAsync(const std::function<void(CURL*)>& configAction) {
        CURL* instance = curl_easy_init();
        std::string result{};

        curl_easy_setopt(instance, CURLOPT_WRITEFUNCTION, curlStringWriter);
        curl_easy_setopt(instance, CURLOPT_WRITEDATA, &result);

        configAction(instance);

        co_await curlEasyPerformAsync(instance);
        curl_easy_cleanup(instance);

        co_return result;
    }
}
