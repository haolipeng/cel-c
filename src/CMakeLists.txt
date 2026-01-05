# CEL 核心库源文件
set(CEL_SOURCES
    cel_error.c
    cel_memory.c
    cel_value.c
    cel_string.c
    cel_bytes.c
    cel_list.c
    cel_map.c
    cel_ast.c
    cel_context.c
    cel_functions.c
    cel_macros.c
    cel_program.c
    parser/lexer.c
    parser/parser.c
    eval/cel_eval.c
    eval/cel_operators.c
    eval/cel_comprehension.c
)

# 创建共享库和静态库
add_library(cel SHARED ${CEL_SOURCES})
add_library(cel_static STATIC ${CEL_SOURCES})

# 链接第三方库
target_link_libraries(cel PRIVATE sds)
target_link_libraries(cel_static PRIVATE sds)

if(CEL_ENABLE_REGEX AND PCRE2_FOUND)
    target_link_libraries(cel PRIVATE PCRE2::PCRE2-8)
    target_link_libraries(cel_static PRIVATE PCRE2::PCRE2-8)
endif()

if(CEL_ENABLE_JSON)
    target_link_libraries(cel PRIVATE cjson)
    target_link_libraries(cel_static PRIVATE cjson)
endif()

# 编译定义
if(CEL_ENABLE_CHRONO)
    target_compile_definitions(cel PRIVATE CEL_ENABLE_CHRONO)
    target_compile_definitions(cel_static PRIVATE CEL_ENABLE_CHRONO)
endif()

if(CEL_THREAD_SAFE)
    target_compile_definitions(cel PRIVATE CEL_THREAD_SAFE)
    target_compile_definitions(cel_static PRIVATE CEL_THREAD_SAFE)
    target_link_libraries(cel PRIVATE pthread)
    target_link_libraries(cel_static PRIVATE pthread)
endif()

# 设置输出名称
set_target_properties(cel_static PROPERTIES OUTPUT_NAME cel)
