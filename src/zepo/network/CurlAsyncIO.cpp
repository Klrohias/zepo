//
// Created by qingy on 2024/7/15.
//

#include "CurlAsyncIO.hpp"

#include "zepo/diagnostics/PerfDiagnostics.hpp"

namespace zepo::async_io {
    static size_t curlStringWriter(void* buffer, size_t size, size_t count, void* result) {
        static_cast<std::string*>(result)->append(static_cast<char*>(buffer), size * count);
        return size * count;
    }

    Task<> curlExecuteAsync(const std::function<void(CURL*)>& configAction) {
        CURL* instance = curl_easy_init();
        try {
            configAction(instance);
            co_await curlEasyPerformAsync(instance);
            curl_easy_cleanup(instance);
        } catch (...) {
            const auto exception = std::current_exception();
            curl_easy_cleanup(instance);
            std::rethrow_exception(exception);
        }
    }

    Task<> async_io::curlEasyPerformAsync(CURL* curlInstance) {
        co_await TaskUtils::run<void>([curlInstance] {
            curl_easy_perform(curlInstance);
        });
    }

    Task<> curlEasyPerformAsync(const std::shared_ptr<CURL>& curlInstance) {
        co_await curlEasyPerformAsync(curlInstance.get());
    }

    Task<std::string> curlExecuteStringAsync(const std::function<void(CURL*)>& configAction) {
        std::string result{};

        co_await curlExecuteAsync([&] (CURL* instance){
            curl_easy_setopt(instance, CURLOPT_WRITEFUNCTION, curlStringWriter);
            curl_easy_setopt(instance, CURLOPT_WRITEDATA, &result);
            configAction(instance);
        });

        co_return result;
    }
}
