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