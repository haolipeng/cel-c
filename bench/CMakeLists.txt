# CEL-C 基准测试
add_executable(bench_cel bench_cel.c)
target_link_libraries(bench_cel PRIVATE cel_static m)
target_include_directories(bench_cel PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
