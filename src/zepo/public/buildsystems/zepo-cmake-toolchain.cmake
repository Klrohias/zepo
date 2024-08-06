#
# Use with -DCMAKE_TOOLCHAIN_FILE=/path/to/zepo-cmake-toolchain.cmake
#

set(ZEPO_ENTRY "${CMAKE_CURRENT_LIST_DIR}/../zepo")

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/zepo_packages")

macro (zepo_generate)
    message(STATUS "Running zepo generate")
    execute_process(
            COMMAND ${ZEPO_ENTRY} generate cmake
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY
    )
endmacro ()

if (DEFINED ZEPO_NEXT_TOOLCHAIN)
    include(${ZEPO_NEXT_TOOLCHAIN})
endif ()