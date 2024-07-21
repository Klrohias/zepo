cmake_minimum_required(VERSION 3.28)
project(zepo)

include_directories(src)

add_subdirectory(thirdparty/semver)
add_subdirectory(thirdparty/quickjs)
add_subdirectory(src/zepo)
