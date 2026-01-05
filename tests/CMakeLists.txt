# 测试可执行文件列表
set(TESTS
    test_error
    test_memory
    # 后续添加更多测试...
)

# 为每个测试创建可执行文件
foreach(test_name ${TESTS})
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} PRIVATE cel_static unity)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 添加测试覆盖率目标 (需要 gcov)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV gcov)
    find_program(LCOV lcov)

    if(GCOV AND LCOV)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
            COMMAND lcov --directory . --zerocounters
            COMMAND ctest
            COMMAND lcov --directory . --capture --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' '*/third_party/*' '*/tests/*' --output-file coverage.info
            COMMAND lcov --list coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating test coverage report"
        )
    endif()
endif()

# Valgrind 内存检查
find_program(VALGRIND valgrind)
if(VALGRIND)
    add_custom_target(memcheck
        COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process
            --test-action memcheck
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests with Valgrind"
    )

    set(MEMORYCHECK_COMMAND_OPTIONS
        "--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
    )
endif()
