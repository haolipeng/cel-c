# Phase 1 实施指南 - 基础设施层

本文档为 CEL-C 项目 Phase 1 (基础设施层) 提供详细的实施指导，包括具体的代码模板、配置示例和验收标准。

---

## 📋 Phase 1 概览

**目标**: 搭建项目基础设施，为后续开发提供构建、错误处理、内存管理和测试支持。

**时间线**: Week 1-2 (约 10 个工作日)

**任务列表**:
- Task 1.1: 项目结构与构建系统 (2-3 天) - P0
- Task 1.2: 错误处理模块 (2-3 天) - P0
- Task 1.3: 内存管理模块 (3-4 天) - P0
- Task 1.4: 测试框架集成 (2 天) - P1

**并行执行**: 4 个任务可以完全并行，由 4 位工程师同时进行

---

## Task 1.1: 项目结构与构建系统

### 负责人
DevOps 工程师 或 构建工程师

### 预计工时
2-3 天

### 详细步骤

#### 第 1 步: 创建项目目录结构 (1 小时)

```bash
mkdir -p cel-c
cd cel-c

# 创建目录结构
mkdir -p include/cel
mkdir -p src/parser
mkdir -p src/eval
mkdir -p tests
mkdir -p third_party
mkdir -p bench
mkdir -p docs
mkdir -p examples
```

#### 第 2 步: 创建主 CMakeLists.txt (2-3 小时)

**文件**: `cel-c/CMakeLists.txt`

```cmake
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

# 安装规则
install(DIRECTORY include/cel DESTINATION include)
install(TARGETS cel DESTINATION lib)
```

#### 第 3 步: 配置第三方库 (2-3 小时)

**文件**: `cel-c/third_party/CMakeLists.txt`

```cmake
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

# PCRE2 (可选)
if(CEL_ENABLE_REGEX)
    find_package(PCRE2 REQUIRED)
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
    add_subdirectory(cjson)
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
```

#### 第 4 步: 创建主库 CMakeLists.txt (1 小时)

**文件**: `cel-c/src/CMakeLists.txt`

```cmake
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

if(CEL_ENABLE_REGEX)
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
```

#### 第 5 步: 创建配置文件 (30 分钟)

**文件**: `cel-c/.gitignore`

```gitignore
# Build artifacts
build/
cmake-build-*/
*.o
*.a
*.so
*.dylib

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# Test outputs
tests/*.log
tests/*.xml

# Third-party downloads
third_party/uthash/
third_party/sds/
third_party/cjson/
third_party/unity/
```

**文件**: `cel-c/README.md`

```markdown
# CEL-C

C11 implementation of Common Expression Language (CEL).

## Building

### Prerequisites

- CMake 3.15+
- C11-compatible compiler (GCC 7+, Clang 6+, MSVC 2019+)
- PCRE2 (optional, for regex support)

### Quick Start

```bash
mkdir build && cd build
cmake ..
make
make test
```

### Build Options

- `CEL_ENABLE_CHRONO`: Enable timestamp/duration support (default: ON)
- `CEL_ENABLE_REGEX`: Enable regex support (default: ON)
- `CEL_ENABLE_JSON`: Enable JSON conversion (default: OFF)
- `CEL_THREAD_SAFE`: Enable thread-safe ref counting (default: ON)
- `CEL_USE_ASAN`: Enable AddressSanitizer (default: OFF)

Example:
```bash
cmake -DCEL_USE_ASAN=ON -DCEL_ENABLE_JSON=ON ..
```

## Usage

```c
#include <cel/cel.h>

int main() {
    // TODO: Add usage example after Phase 2
    return 0;
}
```

## License

Apache 2.0
```

#### 第 6 步: 验证构建系统 (1 小时)

```bash
cd cel-c
mkdir build && cd build
cmake ..
# 此时应该成功配置，虽然源文件还不存在
```

### 验收标准

- [ ] 目录结构完整创建
- [ ] CMakeLists.txt 配置正确，可以成功运行 `cmake ..`
- [ ] 第三方库自动下载/克隆成功
- [ ] `.gitignore` 配置完整
- [ ] README.md 包含构建说明
- [ ] 构建系统支持 Debug/Release 模式
- [ ] 可以配置特性开关 (chrono, regex, JSON 等)
- [ ] AddressSanitizer 可选启用

### 交付物

```
cel-c/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/cel/
├── src/
│   ├── CMakeLists.txt
│   ├── parser/
│   └── eval/
├── tests/
├── third_party/
│   └── CMakeLists.txt
├── bench/
├── docs/
└── examples/
```

---

## Task 1.2: 错误处理模块

### 负责人
核心库工程师 A

### 预计工时
2-3 天

### 详细步骤

#### 第 1 步: 定义错误码和结构 (1 小时)

**文件**: `cel-c/include/cel/cel_error.h`

```c
#ifndef CEL_ERROR_H
#define CEL_ERROR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CEL 错误码枚举
 */
typedef enum {
    CEL_OK = 0,                     // 成功
    CEL_ERR_PARSE,                  // 解析错误
    CEL_ERR_EVAL,                   // 求值错误
    CEL_ERR_TYPE,                   // 类型错误
    CEL_ERR_OVERFLOW,               // 整数溢出
    CEL_ERR_DIV_ZERO,               // 除零错误
    CEL_ERR_INDEX_OUT_OF_BOUNDS,    // 索引越界
    CEL_ERR_NO_SUCH_KEY,            // Map 键不存在
    CEL_ERR_UNDEFINED_VAR,          // 未定义变量
    CEL_ERR_UNDEFINED_FUNC,         // 未定义函数
    CEL_ERR_INVALID_ARG_COUNT,      // 参数数量错误
    CEL_ERR_NO_SUCH_OVERLOAD,       // 函数重载不匹配
    CEL_ERR_FUNC_ERROR,             // 函数执行错误
    CEL_ERR_NOMEM,                  // 内存不足
    CEL_ERR_MAX_RECURSION,          // 超过最大递归深度
    CEL_ERR_INVALID_UTF8,           // 无效的 UTF-8 字符串
    CEL_ERR_INVALID_ARGUMENT        // 无效参数
} cel_error_code_e;

/**
 * @brief 错误结构体
 */
typedef struct {
    cel_error_code_e code;          // 错误码
    char* message;                  // 错误消息 (动态分配)
    uint64_t source_position;       // 源代码位置 (AST node id)
    char* source_snippet;           // 源代码片段 (可选,动态分配)
} cel_error_t;

/**
 * @brief 前向声明 (cel_value_t 在 cel_value.h 中定义)
 */
struct cel_value;

/**
 * @brief Result 类型 (类似 Rust 的 Result<T, E>)
 */
typedef struct {
    bool success;                   // true = 成功, false = 失败
    union {
        struct cel_value* value;    // 成功时的值
        cel_error_t* error;         // 失败时的错误
    } data;
} cel_result_t;

/**
 * @brief 创建错误对象
 *
 * @param code 错误码
 * @param message 错误消息 (会被复制)
 * @return cel_error_t* 错误对象 (需要调用 cel_error_destroy 释放)
 */
cel_error_t* cel_error_create(cel_error_code_e code, const char* message);

/**
 * @brief 创建带位置信息的错误
 */
cel_error_t* cel_error_create_with_pos(
    cel_error_code_e code,
    const char* message,
    uint64_t position
);

/**
 * @brief 销毁错误对象
 */
void cel_error_destroy(cel_error_t* error);

/**
 * @brief 获取错误码的字符串表示
 */
const char* cel_error_code_string(cel_error_code_e code);

/**
 * @brief 创建成功的 Result
 *
 * @param value 返回的值 (所有权转移)
 * @return cel_result_t
 */
cel_result_t cel_ok_result(struct cel_value* value);

/**
 * @brief 创建失败的 Result
 *
 * @param code 错误码
 * @param message 错误消息
 * @return cel_result_t
 */
cel_result_t cel_error_result(cel_error_code_e code, const char* message);

/**
 * @brief 创建失败的 Result (从已存在的 error 对象)
 *
 * @param error 错误对象 (所有权转移)
 * @return cel_result_t
 */
cel_result_t cel_error_result_from(cel_error_t* error);

/**
 * @brief 销毁 Result
 *
 * 会自动销毁包含的 value 或 error
 */
void cel_result_destroy(cel_result_t* result);

/**
 * @brief 宏: 错误传播
 *
 * 如果表达式返回错误,立即返回该错误
 *
 * 使用示例:
 * ```c
 * CEL_TRY(some_function_that_returns_result());
 * ```
 */
#define CEL_TRY(expr) \
    do { \
        cel_result_t __cel_result = (expr); \
        if (!__cel_result.success) { \
            return __cel_result; \
        } \
    } while (0)

/**
 * @brief 宏: 展开 Result 到变量
 *
 * 如果 Result 是错误,返回错误;如果成功,将值赋给变量
 *
 * 使用示例:
 * ```c
 * cel_value_t* val;
 * CEL_UNWRAP(some_function(), val);
 * // 现在可以使用 val
 * ```
 */
#define CEL_UNWRAP(result_expr, var) \
    do { \
        cel_result_t __cel_unwrap_result = (result_expr); \
        if (!__cel_unwrap_result.success) { \
            return __cel_unwrap_result; \
        } \
        var = __cel_unwrap_result.data.value; \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // CEL_ERROR_H
```

#### 第 2 步: 实现错误处理函数 (2-3 小时)

**文件**: `cel-c/src/cel_error.c`

```c
#include "cel/cel_error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

cel_error_t* cel_error_create(cel_error_code_e code, const char* message) {
    cel_error_t* error = (cel_error_t*)malloc(sizeof(cel_error_t));
    if (!error) return NULL;

    error->code = code;
    error->message = message ? strdup(message) : NULL;
    error->source_position = 0;
    error->source_snippet = NULL;

    return error;
}

cel_error_t* cel_error_create_with_pos(
    cel_error_code_e code,
    const char* message,
    uint64_t position
) {
    cel_error_t* error = cel_error_create(code, message);
    if (error) {
        error->source_position = position;
    }
    return error;
}

void cel_error_destroy(cel_error_t* error) {
    if (!error) return;

    free(error->message);
    free(error->source_snippet);
    free(error);
}

const char* cel_error_code_string(cel_error_code_e code) {
    switch (code) {
        case CEL_OK: return "OK";
        case CEL_ERR_PARSE: return "ParseError";
        case CEL_ERR_EVAL: return "EvalError";
        case CEL_ERR_TYPE: return "TypeError";
        case CEL_ERR_OVERFLOW: return "OverflowError";
        case CEL_ERR_DIV_ZERO: return "DivisionByZero";
        case CEL_ERR_INDEX_OUT_OF_BOUNDS: return "IndexOutOfBounds";
        case CEL_ERR_NO_SUCH_KEY: return "NoSuchKey";
        case CEL_ERR_UNDEFINED_VAR: return "UndefinedVariable";
        case CEL_ERR_UNDEFINED_FUNC: return "UndefinedFunction";
        case CEL_ERR_INVALID_ARG_COUNT: return "InvalidArgumentCount";
        case CEL_ERR_NO_SUCH_OVERLOAD: return "NoSuchOverload";
        case CEL_ERR_FUNC_ERROR: return "FunctionError";
        case CEL_ERR_NOMEM: return "OutOfMemory";
        case CEL_ERR_MAX_RECURSION: return "MaxRecursionExceeded";
        case CEL_ERR_INVALID_UTF8: return "InvalidUTF8";
        case CEL_ERR_INVALID_ARGUMENT: return "InvalidArgument";
        default: return "UnknownError";
    }
}

cel_result_t cel_ok_result(struct cel_value* value) {
    cel_result_t result;
    result.success = true;
    result.data.value = value;
    return result;
}

cel_result_t cel_error_result(cel_error_code_e code, const char* message) {
    cel_result_t result;
    result.success = false;
    result.data.error = cel_error_create(code, message);
    return result;
}

cel_result_t cel_error_result_from(cel_error_t* error) {
    cel_result_t result;
    result.success = false;
    result.data.error = error;
    return result;
}

void cel_result_destroy(cel_result_t* result) {
    if (!result) return;

    if (result->success) {
        // 注意: 这里不释放 value,因为它可能还在使用中
        // 由调用者负责管理 value 的生命周期
    } else {
        cel_error_destroy(result->data.error);
    }
}
```

#### 第 3 步: 编写单元测试 (2-3 小时)

**文件**: `cel-c/tests/test_error.c`

```c
#include "unity.h"
#include "cel/cel_error.h"

void setUp(void) {
    // 每个测试前执行
}

void tearDown(void) {
    // 每个测试后执行
}

// 测试错误创建
void test_error_create(void) {
    cel_error_t* error = cel_error_create(CEL_ERR_TYPE, "Type mismatch");

    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL(CEL_ERR_TYPE, error->code);
    TEST_ASSERT_EQUAL_STRING("Type mismatch", error->message);
    TEST_ASSERT_EQUAL(0, error->source_position);

    cel_error_destroy(error);
}

// 测试带位置的错误
void test_error_with_position(void) {
    cel_error_t* error = cel_error_create_with_pos(
        CEL_ERR_PARSE,
        "Unexpected token",
        42
    );

    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL(CEL_ERR_PARSE, error->code);
    TEST_ASSERT_EQUAL(42, error->source_position);

    cel_error_destroy(error);
}

// 测试错误码字符串
void test_error_code_string(void) {
    TEST_ASSERT_EQUAL_STRING("OK", cel_error_code_string(CEL_OK));
    TEST_ASSERT_EQUAL_STRING("TypeError", cel_error_code_string(CEL_ERR_TYPE));
    TEST_ASSERT_EQUAL_STRING("DivisionByZero", cel_error_code_string(CEL_ERR_DIV_ZERO));
}

// 测试 Result 成功情况
void test_ok_result(void) {
    // 注意: 这里使用 NULL 模拟 cel_value_t*,实际使用需要 Phase 2
    cel_result_t result = cel_ok_result(NULL);

    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_NULL(result.data.value);
}

// 测试 Result 失败情况
void test_error_result(void) {
    cel_result_t result = cel_error_result(CEL_ERR_OVERFLOW, "Integer overflow");

    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_NOT_NULL(result.data.error);
    TEST_ASSERT_EQUAL(CEL_ERR_OVERFLOW, result.data.error->code);
    TEST_ASSERT_EQUAL_STRING("Integer overflow", result.data.error->message);

    cel_result_destroy(&result);
}

// 测试 NULL 消息
void test_error_null_message(void) {
    cel_error_t* error = cel_error_create(CEL_ERR_NOMEM, NULL);

    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL(CEL_ERR_NOMEM, error->code);
    TEST_ASSERT_NULL(error->message);

    cel_error_destroy(error);
}

// 测试多次销毁 (应该安全)
void test_error_double_destroy(void) {
    cel_error_t* error = cel_error_create(CEL_ERR_TYPE, "Test");
    cel_error_destroy(error);
    cel_error_destroy(NULL); // 应该不崩溃
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_error_create);
    RUN_TEST(test_error_with_position);
    RUN_TEST(test_error_code_string);
    RUN_TEST(test_ok_result);
    RUN_TEST(test_error_result);
    RUN_TEST(test_error_null_message);
    RUN_TEST(test_error_double_destroy);

    return UNITY_END();
}
```

**文件**: `cel-c/tests/CMakeLists.txt`

```cmake
# 测试可执行文件
add_executable(test_error test_error.c)
target_link_libraries(test_error PRIVATE cel_static unity)

# 注册测试
add_test(NAME test_error COMMAND test_error)
```

#### 第 4 步: 验证测试 (30 分钟)

```bash
cd build
cmake -DCEL_BUILD_TESTS=ON ..
make test_error
./tests/test_error
```

### 验收标准

- [ ] `cel_error.h` 定义完整,包含所有错误码
- [ ] `cel_error.c` 实现所有函数
- [ ] 单元测试覆盖所有错误处理函数
- [ ] 测试通过率 100%
- [ ] 没有内存泄漏 (使用 `valgrind ./tests/test_error` 验证)
- [ ] CEL_TRY 和 CEL_UNWRAP 宏正确工作

### 交付物

```
include/cel/cel_error.h
src/cel_error.c
tests/test_error.c
tests/CMakeLists.txt
```

---

## Task 1.3: 内存管理模块

### 负责人
核心库工程师 B

### 预计工时
3-4 天

### 详细步骤

#### 第 1 步: 定义 Arena 分配器接口 (1 小时)

**文件**: `cel-c/include/cel/cel_memory.h`

```c
#ifndef CEL_MEMORY_H
#define CEL_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Arena 分配器 (内存池)
 *
 * Arena 用于快速分配多个小对象,销毁时一次性释放所有对象。
 * 特别适合 AST 节点分配。
 */
typedef struct arena arena_t;

/**
 * @brief 创建 Arena
 *
 * @param initial_capacity 初始容量 (字节)
 * @return arena_t* Arena 对象,失败返回 NULL
 */
arena_t* arena_create(size_t initial_capacity);

/**
 * @brief 从 Arena 分配内存
 *
 * @param arena Arena 对象
 * @param size 分配大小 (字节)
 * @param alignment 对齐要求 (通常为 8)
 * @return void* 分配的内存,失败返回 NULL
 */
void* arena_alloc(arena_t* arena, size_t size, size_t alignment);

/**
 * @brief 重置 Arena (清空所有分配,但保留内存池)
 *
 * @param arena Arena 对象
 */
void arena_reset(arena_t* arena);

/**
 * @brief 销毁 Arena (释放所有内存)
 *
 * @param arena Arena 对象
 */
void arena_destroy(arena_t* arena);

/**
 * @brief 获取 Arena 统计信息
 */
typedef struct {
    size_t total_allocated;     // 总分配字节数
    size_t total_capacity;      // 总容量
    size_t block_count;         // 内存块数量
} arena_stats_t;

arena_stats_t arena_get_stats(const arena_t* arena);

/**
 * @brief 便捷宏: 分配类型
 */
#define ARENA_ALLOC(arena, type) \
    ((type*)arena_alloc(arena, sizeof(type), _Alignof(type)))

#define ARENA_ALLOC_ARRAY(arena, type, count) \
    ((type*)arena_alloc(arena, sizeof(type) * (count), _Alignof(type)))

/**
 * @brief 引用计数辅助宏
 *
 * 用于实现引用计数的对象
 */

// 单线程版本
#define REF_COUNT_INIT(obj) ((obj)->ref_count = 1)
#define REF_COUNT_RETAIN(obj) ((obj)->ref_count++)
#define REF_COUNT_RELEASE(obj) (--(obj)->ref_count)

// 多线程版本 (需要 CEL_THREAD_SAFE)
#ifdef CEL_THREAD_SAFE
#include <stdatomic.h>
#define ATOMIC_REF_COUNT_INIT(obj) atomic_init(&(obj)->ref_count, 1)
#define ATOMIC_REF_COUNT_RETAIN(obj) atomic_fetch_add(&(obj)->ref_count, 1)
#define ATOMIC_REF_COUNT_RELEASE(obj) atomic_fetch_sub(&(obj)->ref_count, 1)
#endif

#ifdef __cplusplus
}
#endif

#endif // CEL_MEMORY_H
```

#### 第 2 步: 实现 Arena 分配器 (4-5 小时)

**文件**: `cel-c/src/cel_memory.c`

```c
#include "cel/cel_memory.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Arena 内存块
typedef struct arena_block {
    uint8_t* memory;            // 内存区域
    size_t capacity;            // 容量
    size_t offset;              // 当前分配偏移
    struct arena_block* next;   // 下一个块
} arena_block_t;

// Arena 结构
struct arena {
    arena_block_t* current;     // 当前块
    arena_block_t* first;       // 第一个块 (用于 reset)
    size_t default_block_size;  // 默认块大小
};

// 创建新的内存块
static arena_block_t* arena_block_create(size_t capacity) {
    arena_block_t* block = (arena_block_t*)malloc(sizeof(arena_block_t));
    if (!block) return NULL;

    block->memory = (uint8_t*)malloc(capacity);
    if (!block->memory) {
        free(block);
        return NULL;
    }

    block->capacity = capacity;
    block->offset = 0;
    block->next = NULL;

    return block;
}

// 销毁内存块
static void arena_block_destroy(arena_block_t* block) {
    while (block) {
        arena_block_t* next = block->next;
        free(block->memory);
        free(block);
        block = next;
    }
}

// 对齐辅助函数
static inline size_t align_up(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

arena_t* arena_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 4096; // 默认 4KB
    }

    arena_t* arena = (arena_t*)malloc(sizeof(arena_t));
    if (!arena) return NULL;

    arena->first = arena_block_create(initial_capacity);
    if (!arena->first) {
        free(arena);
        return NULL;
    }

    arena->current = arena->first;
    arena->default_block_size = initial_capacity;

    return arena;
}

void* arena_alloc(arena_t* arena, size_t size, size_t alignment) {
    if (!arena || size == 0) return NULL;

    // 对齐当前偏移
    size_t aligned_offset = align_up(arena->current->offset, alignment);
    size_t required = aligned_offset + size;

    // 检查当前块是否有足够空间
    if (required > arena->current->capacity) {
        // 需要新块
        size_t new_block_size = arena->default_block_size;
        if (size > new_block_size) {
            new_block_size = size * 2; // 确保能容纳
        }

        arena_block_t* new_block = arena_block_create(new_block_size);
        if (!new_block) return NULL;

        // 链接到当前块
        arena->current->next = new_block;
        arena->current = new_block;

        // 重新对齐
        aligned_offset = 0;
    }

    // 分配
    void* ptr = arena->current->memory + aligned_offset;
    arena->current->offset = aligned_offset + size;

    return ptr;
}

void arena_reset(arena_t* arena) {
    if (!arena) return;

    // 重置所有块的偏移
    arena_block_t* block = arena->first;
    while (block) {
        block->offset = 0;
        block = block->next;
    }

    // 恢复当前块为第一个块
    arena->current = arena->first;
}

void arena_destroy(arena_t* arena) {
    if (!arena) return;

    arena_block_destroy(arena->first);
    free(arena);
}

arena_stats_t arena_get_stats(const arena_t* arena) {
    arena_stats_t stats = {0, 0, 0};
    if (!arena) return stats;

    const arena_block_t* block = arena->first;
    while (block) {
        stats.total_allocated += block->offset;
        stats.total_capacity += block->capacity;
        stats.block_count++;
        block = block->next;
    }

    return stats;
}
```

#### 第 3 步: 编写单元测试 (2-3 小时)

**文件**: `cel-c/tests/test_memory.c`

```c
#include "unity.h"
#include "cel/cel_memory.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// 测试 Arena 创建和销毁
void test_arena_create_destroy(void) {
    arena_t* arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena);

    arena_stats_t stats = arena_get_stats(arena);
    TEST_ASSERT_EQUAL(0, stats.total_allocated);
    TEST_ASSERT_EQUAL(1024, stats.total_capacity);
    TEST_ASSERT_EQUAL(1, stats.block_count);

    arena_destroy(arena);
}

// 测试简单分配
void test_arena_simple_alloc(void) {
    arena_t* arena = arena_create(1024);

    int* p1 = (int*)arena_alloc(arena, sizeof(int), sizeof(int));
    TEST_ASSERT_NOT_NULL(p1);
    *p1 = 42;
    TEST_ASSERT_EQUAL(42, *p1);

    int* p2 = (int*)arena_alloc(arena, sizeof(int), sizeof(int));
    TEST_ASSERT_NOT_NULL(p2);
    *p2 = 100;
    TEST_ASSERT_EQUAL(100, *p2);
    TEST_ASSERT_EQUAL(42, *p1); // p1 应该不变

    arena_destroy(arena);
}

// 测试大量分配
void test_arena_many_allocs(void) {
    arena_t* arena = arena_create(128); // 小容量,会触发多个块

    #define COUNT 100
    int* ptrs[COUNT];

    for (int i = 0; i < COUNT; i++) {
        ptrs[i] = (int*)arena_alloc(arena, sizeof(int), sizeof(int));
        TEST_ASSERT_NOT_NULL(ptrs[i]);
        *ptrs[i] = i;
    }

    // 验证所有值
    for (int i = 0; i < COUNT; i++) {
        TEST_ASSERT_EQUAL(i, *ptrs[i]);
    }

    arena_stats_t stats = arena_get_stats(arena);
    TEST_ASSERT_GREATER_THAN(0, stats.block_count);

    arena_destroy(arena);
}

// 测试 Arena 重置
void test_arena_reset(void) {
    arena_t* arena = arena_create(1024);

    // 分配一些内存
    for (int i = 0; i < 10; i++) {
        arena_alloc(arena, 100, 8);
    }

    arena_stats_t stats1 = arena_get_stats(arena);
    TEST_ASSERT_EQUAL(1000, stats1.total_allocated);

    // 重置
    arena_reset(arena);

    arena_stats_t stats2 = arena_get_stats(arena);
    TEST_ASSERT_EQUAL(0, stats2.total_allocated);
    TEST_ASSERT_EQUAL(stats1.total_capacity, stats2.total_capacity); // 容量不变

    arena_destroy(arena);
}

// 测试对齐
void test_arena_alignment(void) {
    arena_t* arena = arena_create(1024);

    // 分配 1 字节,对齐到 8
    char* p1 = (char*)arena_alloc(arena, 1, 1);
    int* p2 = (int*)arena_alloc(arena, sizeof(int), 8);

    // 检查 p2 是 8 的倍数
    TEST_ASSERT_EQUAL(0, ((uintptr_t)p2) % 8);

    arena_destroy(arena);
}

// 测试宏
void test_arena_macros(void) {
    arena_t* arena = arena_create(1024);

    typedef struct {
        int x;
        int y;
    } Point;

    Point* p1 = ARENA_ALLOC(arena, Point);
    TEST_ASSERT_NOT_NULL(p1);
    p1->x = 10;
    p1->y = 20;

    Point* arr = ARENA_ALLOC_ARRAY(arena, Point, 5);
    TEST_ASSERT_NOT_NULL(arr);
    for (int i = 0; i < 5; i++) {
        arr[i].x = i;
        arr[i].y = i * 2;
    }

    TEST_ASSERT_EQUAL(3, arr[3].x);
    TEST_ASSERT_EQUAL(6, arr[3].y);

    arena_destroy(arena);
}

// 测试零分配
void test_arena_zero_alloc(void) {
    arena_t* arena = arena_create(1024);

    void* p = arena_alloc(arena, 0, 8);
    TEST_ASSERT_NULL(p);

    arena_destroy(arena);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_arena_create_destroy);
    RUN_TEST(test_arena_simple_alloc);
    RUN_TEST(test_arena_many_allocs);
    RUN_TEST(test_arena_reset);
    RUN_TEST(test_arena_alignment);
    RUN_TEST(test_arena_macros);
    RUN_TEST(test_arena_zero_alloc);

    return UNITY_END();
}
```

#### 第 4 步: 性能测试 (可选,1-2 小时)

**文件**: `cel-c/bench/bench_arena.c`

```c
#include "cel/cel_memory.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define ITERATIONS 1000000

double benchmark_arena() {
    arena_t* arena = arena_create(4096);

    clock_t start = clock();

    for (int i = 0; i < ITERATIONS; i++) {
        arena_alloc(arena, 64, 8);
        if (i % 10000 == 0) {
            arena_reset(arena);
        }
    }

    clock_t end = clock();
    arena_destroy(arena);

    return (double)(end - start) / CLOCKS_PER_SEC;
}

double benchmark_malloc() {
    void* ptrs[10000];

    clock_t start = clock();

    for (int i = 0; i < ITERATIONS; i++) {
        ptrs[i % 10000] = malloc(64);
        if (i % 10000 == 9999) {
            for (int j = 0; j < 10000; j++) {
                free(ptrs[j]);
            }
        }
    }

    clock_t end = clock();

    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main() {
    printf("Arena benchmark: %d iterations\n", ITERATIONS);

    double arena_time = benchmark_arena();
    printf("Arena:  %.3f seconds\n", arena_time);

    double malloc_time = benchmark_malloc();
    printf("Malloc: %.3f seconds\n", malloc_time);

    printf("Speedup: %.2fx\n", malloc_time / arena_time);

    return 0;
}
```

### 验收标准

- [ ] Arena 分配器正确实现
- [ ] 支持多个内存块链接
- [ ] 对齐处理正确
- [ ] `arena_reset` 可以重用内存
- [ ] 单元测试全部通过
- [ ] 没有内存泄漏
- [ ] 宏定义工作正常
- [ ] (可选) Arena 比 malloc 快 2-5 倍

### 交付物

```
include/cel/cel_memory.h
src/cel_memory.c
tests/test_memory.c
bench/bench_arena.c (可选)
```

---

## Task 1.4: 测试框架集成

### 负责人
测试工程师

### 预计工时
2 天

### 详细步骤

#### 第 1 步: 配置 Unity 测试框架 (已在 Task 1.1 完成)

验证 Unity 已正确集成:

```bash
cd cel-c/build
cmake ..
# 应该看到 Unity 被克隆和编译
```

#### 第 2 步: 创建测试辅助宏 (1 小时)

**文件**: `cel-c/tests/test_helpers.h`

```c
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "unity.h"
#include <string.h>

/**
 * @brief 测试辅助宏
 */

// 测试内存相等
#define TEST_ASSERT_MEMORY_EQUAL(expected, actual, length) \
    TEST_ASSERT_EQUAL_MEMORY(expected, actual, length)

// 测试字符串相等 (支持 NULL)
#define TEST_ASSERT_STRING_EQUAL_OR_NULL(expected, actual) \
    do { \
        if ((expected) == NULL && (actual) == NULL) { \
            /* 都是 NULL,通过 */ \
        } else if ((expected) == NULL || (actual) == NULL) { \
            TEST_FAIL_MESSAGE("One string is NULL, other is not"); \
        } else { \
            TEST_ASSERT_EQUAL_STRING(expected, actual); \
        } \
    } while (0)

// 测试浮点数近似相等
#define TEST_ASSERT_DOUBLE_APPROX(expected, actual, tolerance) \
    TEST_ASSERT_DOUBLE_WITHIN(tolerance, expected, actual)

// 测试指针非 NULL
#define TEST_ASSERT_NOT_NULL_MESSAGE(ptr, msg) \
    TEST_ASSERT_NOT_NULL_MESSAGE(ptr, msg)

#endif // TEST_HELPERS_H
```

#### 第 3 步: 更新测试 CMakeLists.txt (1 小时)

**文件**: `cel-c/tests/CMakeLists.txt`

```cmake
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
```

#### 第 4 步: 创建测试运行脚本 (30 分钟)

**文件**: `cel-c/scripts/run_tests.sh`

```bash
#!/bin/bash

set -e

BUILD_DIR="${1:-build}"
BUILD_TYPE="${2:-Debug}"

echo "Building tests in $BUILD_DIR ($BUILD_TYPE)..."

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCEL_BUILD_TESTS=ON \
      -DCEL_USE_ASAN=ON \
      ..

# 编译
make -j$(nproc)

# 运行测试
echo ""
echo "Running tests..."
ctest --output-on-failure

echo ""
echo "All tests passed!"
```

**文件**: `cel-c/scripts/run_memcheck.sh`

```bash
#!/bin/bash

set -e

BUILD_DIR="${1:-build}"

echo "Running Valgrind memory checks..."

cd "$BUILD_DIR"

# 为每个测试运行 Valgrind
for test_exe in tests/test_*; do
    if [ -x "$test_exe" ]; then
        echo ""
        echo "Checking $test_exe..."
        valgrind --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --error-exitcode=1 \
                 "$test_exe"
    fi
done

echo ""
echo "All memory checks passed!"
```

设置执行权限:

```bash
chmod +x scripts/run_tests.sh
chmod +x scripts/run_memcheck.sh
```

#### 第 5 步: 创建 CI 配置 (可选,1 小时)

**文件**: `cel-c/.github/workflows/ci.yml`

```yaml
name: CI

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies (Ubuntu)
      if: matrix.os == 'ubuntu-latest'
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake valgrind lcov libpcre2-dev

    - name: Install dependencies (macOS)
      if: matrix.os == 'macos-latest'
      run: |
        brew install cmake pcre2

    - name: Build
      run: |
        mkdir build && cd build
        cmake -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
              -DCEL_BUILD_TESTS=ON \
              -DCEL_USE_ASAN=ON \
              ..
        make -j$(nproc)

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure

    - name: Memory check (Ubuntu Debug only)
      if: matrix.os == 'ubuntu-latest' && matrix.build_type == 'Debug'
      run: |
        cd build
        for test in tests/test_*; do
          valgrind --leak-check=full --error-exitcode=1 $test
        done

    - name: Coverage (Ubuntu Debug only)
      if: matrix.os == 'ubuntu-latest' && matrix.build_type == 'Debug'
      run: |
        cd build
        make coverage
```

### 验收标准

- [ ] Unity 测试框架正确集成
- [ ] `test_helpers.h` 提供便捷的测试宏
- [ ] CMakeLists.txt 配置所有测试
- [ ] `ctest` 可以运行所有测试
- [ ] `run_tests.sh` 脚本工作正常
- [ ] `run_memcheck.sh` 可以运行 Valgrind 检查
- [ ] (可选) GitHub Actions CI 配置完成

### 交付物

```
tests/test_helpers.h
tests/CMakeLists.txt
scripts/run_tests.sh
scripts/run_memcheck.sh
.github/workflows/ci.yml (可选)
```

---

## Phase 1 总结

### 完成标准

所有 4 个任务的验收标准都满足:
- ✅ 构建系统可以编译空项目
- ✅ 错误处理模块测试 100% 通过
- ✅ 内存管理模块测试 100% 通过
- ✅ 测试框架可以运行所有测试

### 验证命令

```bash
cd cel-c
./scripts/run_tests.sh build Debug
./scripts/run_memcheck.sh build
```

输出应该类似:

```
Running tests...
test_error ................. Passed
test_memory ................ Passed

All 2 tests passed!

Running Valgrind memory checks...
Checking tests/test_error...
All heap blocks were freed -- no leaks are possible

Checking tests/test_memory...
All heap blocks were freed -- no leaks are possible

All memory checks passed!
```

### 后续步骤

Phase 1 完成后,可以开始 Phase 2 (核心数据结构):
- Task 2.1: 基础值类型 (依赖 Task 1.2, 1.3)
- Task 2.2-2.6: 其他值类型和 AST (依赖 Task 2.1)

详细信息请参考 `TASK-BREAKDOWN.md` 和 `TASK-DEPENDENCIES.md`。

---

## 常见问题

### Q1: 构建失败,提示找不到第三方库

**A**: 确保网络连接正常,CMake 会自动下载 uthash, SDS, Unity。如果下载失败,手动克隆:

```bash
cd cel-c/third_party
git clone https://github.com/antirez/sds.git
git clone https://github.com/ThrowTheSwitch/Unity.git unity
wget https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h -P uthash/
```

### Q2: 测试通过但 Valgrind 报告内存泄漏

**A**: 检查是否在所有错误路径都正确释放了内存。使用 Valgrind 的详细输出:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./tests/test_error
```

### Q3: 如何添加新的测试?

**A**:
1. 在 `tests/` 目录创建 `test_xxx.c`
2. 在 `tests/CMakeLists.txt` 的 `TESTS` 列表添加 `test_xxx`
3. 重新运行 `cmake` 和 `make`

### Q4: 如何在 macOS 上使用 AddressSanitizer?

**A**: macOS 的 ASan 可能需要额外配置:

```bash
cmake -DCEL_USE_ASAN=ON ..
# 如果失败,尝试:
export ASAN_OPTIONS=detect_leaks=0  # macOS 不支持 leak detection
```

---

祝 Phase 1 开发顺利! 🚀
