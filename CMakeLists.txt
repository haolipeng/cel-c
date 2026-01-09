cmake_minimum_required(VERSION 3.15)
project(cel-c
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "CEL (Common Expression Language) implementation in C"
)

# C 标准设置
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O3>
)

# 特性选项
option(CEL_ENABLE_CHRONO "Enable timestamp and duration support" ON)
option(CEL_ENABLE_REGEX "Enable regex support (requires PCRE2)" ON)
option(CEL_ENABLE_JSON "Enable JSON conversion support" OFF)
option(CEL_THREAD_SAFE "Enable thread-safe reference counting" ON)
option(CEL_BUILD_TESTS "Build unit tests" ON)
option(CEL_BUILD_BENCH "Build benchmarks" OFF)
option(CEL_BUILD_EXAMPLES "Build examples" ON)
option(CEL_USE_ASAN "Enable AddressSanitizer" OFF)

# PCRE2 检测
if(CEL_ENABLE_REGEX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PCRE2 IMPORTED_TARGET libpcre2-8)
        if(PCRE2_FOUND)
            add_library(PCRE2::PCRE2-8 ALIAS PkgConfig::PCRE2)
            message(STATUS "PCRE2 found via pkg-config")
        endif()
    endif()
    if(NOT PCRE2_FOUND)
        message(WARNING "PCRE2 not found, regex support disabled")
        set(CEL_ENABLE_REGEX OFF)
    endif()
endif()

# AddressSanitizer
if(CEL_USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# 第三方库
add_subdirectory(third_party)

# 主库
add_subdirectory(src)

# 测试
if(CEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
if(CEL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 基准测试
if(CEL_BUILD_BENCH)
    add_subdirectory(bench)
endif()

# 模糊测试
option(CEL_BUILD_FUZZ "Build fuzzing targets" OFF)
if(CEL_BUILD_FUZZ)
    add_subdirectory(fuzz)
endif()

# 安装规则
install(DIRECTORY include/cel DESTINATION include)
install(TARGETS cel DESTINATION lib)
