# uthash (header-only)
# 下载: https://github.com/troydhanson/uthash/blob/master/src/uthash.h
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/uthash/uthash.h)
    message(STATUS "Downloading uthash...")
    file(DOWNLOAD
        https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h
        ${CMAKE_CURRENT_SOURCE_DIR}/uthash/uthash.h
        SHOW_PROGRESS
    )
endif()

# SDS (Simple Dynamic Strings)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sds)
    message(STATUS "Cloning SDS...")
    execute_process(
        COMMAND git clone https://github.com/antirez/sds.git
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
add_library(sds STATIC sds/sds.c)
target_include_directories(sds PUBLIC sds)
# Disable -Werror for third-party code
target_compile_options(sds PRIVATE -Wno-error)

# PCRE2 (可选)
if(CEL_ENABLE_REGEX)
    find_package(PCRE2 QUIET)
    if(NOT PCRE2_FOUND)
        message(WARNING "PCRE2 not found, regex support will be disabled")
        set(CEL_ENABLE_REGEX OFF CACHE BOOL "Enable regex support" FORCE)
    endif()
endif()

# cJSON (可选)
if(CEL_ENABLE_JSON)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cjson)
        message(STATUS "Cloning cJSON...")
        execute_process(
            COMMAND git clone https://github.com/DaveGamble/cJSON.git cjson
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
    add_subdirectory(cjson EXCLUDE_FROM_ALL)
endif()

# Unity 测试框架
if(CEL_BUILD_TESTS)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unity)
        message(STATUS "Cloning Unity...")
        execute_process(
            COMMAND git clone https://github.com/ThrowTheSwitch/Unity.git unity
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
    add_library(unity STATIC unity/src/unity.c)
    target_include_directories(unity PUBLIC unity/src)
endif()
