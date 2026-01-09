# CEL-C 模糊测试
#
# 需要 clang 和 libFuzzer 支持
# 或者使用 AFL++

# 检查是否有 clang
find_program(CLANG clang)

if(CLANG AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    # libFuzzer 模式
    add_executable(fuzz_parser fuzz_parser.c)
    target_link_libraries(fuzz_parser PRIVATE cel_static sds m)
    target_include_directories(fuzz_parser PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)

    add_executable(fuzz_eval fuzz_eval.c)
    target_link_libraries(fuzz_eval PRIVATE cel_static sds m)
    target_include_directories(fuzz_eval PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(fuzz_eval PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_eval PRIVATE -fsanitize=fuzzer,address)

    message(STATUS "Fuzzing targets enabled (libFuzzer)")
else()
    message(STATUS "Fuzzing targets disabled (requires Clang with libFuzzer)")
endif()
