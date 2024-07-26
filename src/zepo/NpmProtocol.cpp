//
// Created by qingy on 2024/7/25.
//

#include "NpmProtocol.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <fstream>
#include <string>

#include "diagnostics/PerfDiagnostics.hpp"
#include "network/CurlAsyncIO.hpp"
#include "serialize/Json.hpp"
#include "serialize/Serializer.hpp"

namespace zepo {
    inline void createDirectoriesIfNeed(const std::filesystem::path& path) {
        if (!is_directory(path)) {
            create_directories(path);
        }
    }

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

    static int copyEntryData(archive* reader, std::iostream& stream) {
        const void* buffer;
        size_t size;
        la_int64_t offset;

        while (true) {
            auto result = archive_read_data_block(reader, &buffer, &size, &offset);

            if (result == ARCHIVE_EOF)
                return ARCHIVE_OK;

            if (result != ARCHIVE_OK)
                return result;

            stream.seekp(offset, std::ios::beg);
            stream.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(size));
        }
    }


    Task<> npmDecompressArchive(const std::filesystem::path& path, const std::filesystem::path& destination) {
        co_await TaskUtils::run<void>([&] {
            using namespace std::string_literals;
            archive* archiveReader{archive_read_new()};
            archive_entry* entry;
            try {
                archive_read_support_format_tar(archiveReader);
                archive_read_support_filter_all(archiveReader);
                auto result = archive_read_open_filename_w(archiveReader, path.c_str(), 1024 * 16); // 16k
                if (result != ARCHIVE_OK) {
                    throw std::runtime_error("libarchive error: "s + archive_error_string(archiveReader));
                }

                while (true) {
                    result = archive_read_next_header(archiveReader, &entry);
                    if (result == ARCHIVE_EOF) {
                        break;
                    }
                    if (result != ARCHIVE_OK) {
                        throw std::runtime_error("libarchive error: "s + archive_error_string(archiveReader));
                    }

                    if (archive_entry_size(entry) == 0) {
                        continue;
                    }

                    const auto extractPath = destination / archive_entry_pathname(entry);
                    createDirectoriesIfNeed(extractPath.parent_path());

                    std::fstream fileStream{extractPath, std::ios::out | std::ios::binary};
                    if (!fileStream.good()) {
                        throw std::runtime_error("failed to open " + extractPath.string() + " for extracting");
                    }

                    result = copyEntryData(archiveReader, fileStream);
                    if (result != ARCHIVE_OK) {
                        throw std::runtime_error("libarchive error: "s + archive_error_string(archiveReader));
                    }
                }

                archive_read_free(archiveReader);
            } catch (...) {
                const auto exception = std::current_exception();
                archive_read_free(archiveReader);
                std::rethrow_exception(exception);
            }
        });
    }
}
