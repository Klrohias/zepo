
find_package(yyjson CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(LibArchive REQUIRED)
find_package(fmt CONFIG REQUIRED)

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
        js_runtime/JSUtils.hpp
        js_runtime/JSUtils.cpp
        js_runtime/JSEnv.hpp
        js_runtime/JSEnv.cpp
        async/ThreadWorker.cpp
        async/ThreadWorker.hpp
        commands/GenerateCommand.hpp
        commands/GenerateCommand.cpp
        commands/InstallCommand.hpp
        commands/InstallCommand.cpp
        Utils.cpp
        Utils.hpp
        pkg_manager/BuildOptions.hpp
        pkg_manager/Build.hpp
        pkg_manager/Build.cpp
        pkg_manager/BuildReport.hpp
        pkg_manager/BuildReport.cpp
)

target_compile_features(zepo PRIVATE cxx_std_20)
target_link_libraries(zepo PRIVATE fmt::fmt)
target_link_libraries(zepo PRIVATE LibArchive::LibArchive)
target_link_libraries(zepo PRIVATE CURL::libcurl)
target_link_libraries(zepo PRIVATE yyjson::yyjson)
target_link_libraries(zepo PRIVATE semver)
target_link_libraries(zepo PRIVATE quickjs-universal)

add_custom_command(
        TARGET zepo POST_BUILD
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/public/*
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/public
        ${CMAKE_CURRENT_BINARY_DIR})