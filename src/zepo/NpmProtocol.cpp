//
// Created by qingy on 2024/7/25.
//

#include "NpmProtocol.hpp"

#include <span>

#include "diagnostics/PerfDiagnostics.hpp"
#include "network/CurlAsyncIO.hpp"
#include "serialize/Json.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    static size_t curlStreamWriter(const void* data, const size_t size, const size_t count, void* typelessStreamPtr) {
        auto* streamPtr = static_cast<std::iostream*>(typelessStreamPtr);
        streamPtr->write(static_cast<const char*>(data), size * count);
        return size * count;
    }

    inline void configureNpmAuth(CURL* instance, const std::optional<std::string_view>& username,
                                 const std::optional<std::string_view>& password) {
        // configure basic-auth
        if (username.has_value() || password.has_value()) {
            curl_easy_setopt(instance, CURLOPT_HTTPAUTH, static_cast<long>(CURLAUTH_BASIC));
        }

        if (username.has_value()) {
            curl_easy_setopt(instance, CURLOPT_USERNAME, username.value().data());
        }

        if (password.has_value()) {
            curl_easy_setopt(instance, CURLOPT_PASSWORD, password.value().data());
        }
    }

    Task<NpmPackageInfo> npmFetchMetadata(const std::string_view url,
                                          const std::optional<std::string_view> username,
                                          const std::optional<std::string_view> password) {
        ZEPO_PERF_BEGIN_(queryNpmMetadata)
        const auto response = co_await async_io::curlExecuteStringAsync([&](CURL* instance) {
            curl_easy_setopt(instance, CURLOPT_URL, url.data());
            curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
            configureNpmAuth(instance, username, password);
        });
        ZEPO_PERF_END_(queryNpmMetadata)

        ZEPO_PERF_BEGIN_(parseNpmMetadata)
        const auto jsonDoc = JsonDocument{response};
        auto result = parse<NpmPackageInfo>(jsonDoc.getRootToken());
        ZEPO_PERF_END_(parseNpmMetadata)

        co_return result;
    }

    Task<> npmDownloadTarball(const std::string_view url,
                              const std::optional<std::string_view> username,
                              const std::optional<std::string_view> password,
                              std::iostream& output) {
        ZEPO_PERF_BEGIN_(downloadNpmTarball)
        co_await async_io::curlExecuteAsync([&](CURL* instance) {
            curl_easy_setopt(instance, CURLOPT_WRITEFUNCTION, curlStreamWriter);
            curl_easy_setopt(instance, CURLOPT_WRITEDATA, &output);
            curl_easy_setopt(instance, CURLOPT_URL, url.data());
            curl_easy_setopt(instance, CURLOPT_NOSIGNAL, 1);
            curl_easy_setopt(instance, CURLOPT_FOLLOWLOCATION, 1L);
            configureNpmAuth(instance, username, password);
        });
        ZEPO_PERF_END_(downloadNpmTarball)
    }
}
