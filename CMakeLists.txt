cmake_minimum_required(VERSION 3.28)
project(zepo)

include_directories(src)
include(FetchContent)

# External Targets
FetchContent_Declare(
        quickjs-universal
        GIT_REPOSITORY https://github.com/Klrohias/quickjs-universal
        GIT_TAG 560a5ceaf2e8e917d2ad975a733e8e2998d3553f
)

FetchContent_MakeAvailable(quickjs-universal)

include(FetchContent)

FetchContent_Declare(
        semver
        GIT_REPOSITORY https://github.com/h2non/semver.c
        GIT_TAG bd1db234a68f305ed10268bd023df1ad672061d7
)

FetchContent_GetProperties(semver)
if(NOT semver_POPULATED)
    FetchContent_Populate(semver)
endif()

add_library(semver ${semver_SOURCE_DIR}/semver.c ${semver_SOURCE_DIR}/semver.h)
target_include_directories(semver INTERFACE ${semver_SOURCE_DIR})

# Local Targets
add_subdirectory(src/zepo)
