
find_package(yyjson CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(LibArchive REQUIRED)
find_package(uvw CONFIG REQUIRED)
add_executable(zepo main.cpp
        async/Task.hpp
        async/TaskCompletionSource.hpp
        async/TaskUtils.hpp
        async/ThreadPool.cpp
        async/ThreadPool.hpp
        serialize/Json.hpp
        serialize/Serializer.hpp
        serialize/Json.cpp
        serialize/Reflect.hpp
        async/AsyncIO.hpp
        Configuration.hpp
        Manifest.hpp
        NpmProtocol.hpp
        network/CurlAsyncIO.hpp
        network/CurlAsyncIO.cpp
        PackageInstallation.hpp
        PackageInstallation.cpp
        semver/Semver.hpp
        semver/Semver.cpp
        semver/Range.cpp
        semver/Range.hpp
        async/Generator.hpp
        Global.hpp
        Global.cpp
        diagnostics/PerfDiagnostics.cpp
        diagnostics/PerfDiagnostics.hpp
        NpmProtocol.cpp
        Interfaces.hpp
        js_runtime/JSUtils.hpp
        js_runtime/JSUtils.cpp
        js_runtime/JSEnv.hpp
        js_runtime/JSEnv.cpp
        async/ThreadWorker.cpp
        async/ThreadWorker.hpp
)

target_compile_features(zepo PRIVATE cxx_std_20)
target_link_libraries(zepo PRIVATE uvw::uvw)
target_link_libraries(zepo PRIVATE LibArchive::LibArchive)
target_link_libraries(zepo PRIVATE CURL::libcurl)
target_link_libraries(zepo PRIVATE yyjson::yyjson)
target_link_libraries(zepo PRIVATE semver)
target_link_libraries(zepo PRIVATE quickjs-universal)