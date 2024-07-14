
find_package(yyjson CONFIG REQUIRED)

add_executable(zepo main.cpp
    async/Task.hpp async/TaskCompletionSource.hpp async/TaskUtils.hpp async/ThreadPool.cpp async/ThreadPool.hpp
        serialize/Json.hpp
        serialize/Serializer.hpp
        serialize/Json.cpp
        serialize/Reflect.hpp
        async/AsyncIO.hpp)

target_compile_features(zepo PRIVATE cxx_std_20)

target_link_libraries(zepo PRIVATE yyjson::yyjson)