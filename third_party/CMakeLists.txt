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

# PCRE2 (可选) - 检测已在主 CMakeLists.txt 中完成
# 这里不再重复检测

# cJSON (可选)
if(CEL_ENABLE_JSON)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cjson)
        message(STATUS "Cloning cJSON...")
        execute_process(
            COMMAND git clone https://github.com/DaveGamble/cJSON.git cjson
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
    # 禁用 cJSON 测试以避免与 Unity 冲突
    set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
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
