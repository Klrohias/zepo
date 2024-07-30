if (DEFINED ZEPO_NEXT_TOOLCHAIN)
    include(${ZEPO_NEXT_TOOLCHAIN})
endif ()

macro (zepo_find_package exportName packageName)
    message("zepo: find package ${packageName}")
    execute_process(
            COMMAND ${ZEPO_ENTRY} get-package ${packageName} --available
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE ZEPO_OUTPUT
            COMMAND_ERROR_IS_FATAL ANY
    )

    string(JSON ZEPO_PACKAGE_TYPE GET ${ZEPO_OUTPUT} type)

    if (ZEPO_PACKAGE_TYPE STREQUAL "header-only")
        add_library(${exportName} INTERFACE)
        string(JSON ZEPO_PACKAGE_INCLUDES GET ${ZEPO_OUTPUT} paths includes)
        target_include_directories(${exportName} INTERFACE ${ZEPO_PACKAGE_INCLUDES})
    endif ()
endmacro ()