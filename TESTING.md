# 测试指南

本文档说明如何在 CEL-C 项目中编写和运行测试。

---

## 测试框架

CEL-C 使用以下测试工具:

- **Unity** - 轻量级单元测试框架
- **Valgrind** - 内存泄漏检测
- **LCOV** - 代码覆盖率分析
- **AddressSanitizer (ASan)** - 内存错误检测

---

## 运行测试

### 快速运行所有测试

```bash
./scripts/run_tests.sh
```

### 手动运行测试

```bash
# 1. 构建项目 (Debug 模式)
cd /home/work/cel-c
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCEL_BUILD_TESTS=ON ..
make -j$(nproc)

# 2. 运行所有测试
ctest --output-on-failure

# 3. 运行单个测试
./tests/test_error
./tests/test_memory
```

### 内存检查

```bash
# 使用脚本 (推荐)
./scripts/run_memcheck.sh

# 或手动运行
cd build
valgrind --leak-check=full ./tests/test_error
valgrind --leak-check=full ./tests/test_memory
```

### 代码覆盖率

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCEL_BUILD_TESTS=ON ..
make -j$(nproc)
make coverage

# 查看覆盖率报告
lcov --list coverage.info
```

---

## 编写测试

### 测试文件结构

每个测试文件应包含:

```c
#include "cel/cel_xxx.h"   // 被测试的模块
#include "test_helpers.h"   // 测试辅助宏
#include "unity.h"          // Unity 框架

/* Unity 设置 */
void setUp(void) {
    // 每个测试前执行
}

void tearDown(void) {
    // 每个测试后执行
}

/* 测试函数 */
void test_something(void) {
    TEST_ASSERT_EQUAL(expected, actual);
}

/* 主函数 */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_something);
    return UNITY_END();
}
```

### 使用测试辅助宏

`test_helpers.h` 提供了便捷的测试宏:

#### 字符串测试

```c
// 支持 NULL 的字符串比较
TEST_ASSERT_STRING_EQUAL_OR_NULL(expected, actual);

// 断言包含子串
TEST_ASSERT_STRING_CONTAINS("hello world", "world");

// 断言以前缀开头
TEST_ASSERT_STRING_STARTS_WITH("hello world", "hello");
```

#### 浮点数测试

```c
// 近似相等 (相对误差 0.1%)
TEST_ASSERT_DOUBLE_APPROX(3.14159, 3.14160, 0.001);
```

#### 内存测试

```c
// 内存内容相等
TEST_ASSERT_MEMORY_EQUAL(expected_buf, actual_buf, size);

// 内存块全为零
TEST_ASSERT_MEMORY_ZERO(buffer, 100);
```

#### 范围测试

```c
// 值在范围内 [10, 20]
TEST_ASSERT_IN_RANGE(value, 10, 20);

// 值不在范围内
TEST_ASSERT_NOT_IN_RANGE(value, 10, 20);
```

#### 数组测试

```c
int expected[] = {1, 2, 3};
int actual[] = {1, 2, 3};
TEST_ASSERT_INT_ARRAY_EQUAL(expected, actual, 3);
```

#### CEL Result 测试

```c
cel_result_t result = some_function();

// 断言成功
TEST_ASSERT_RESULT_OK(result);

// 断言失败
TEST_ASSERT_RESULT_ERROR(result);

// 断言特定错误码
TEST_ASSERT_RESULT_ERROR_CODE(result, CEL_ERROR_SYNTAX);
```

---

## 添加新测试

### 步骤 1: 创建测试文件

在 `tests/` 目录创建 `test_xxx.c`:

```bash
vim tests/test_my_module.c
```

### 步骤 2: 编写测试

```c
#include "cel/cel_my_module.h"
#include "test_helpers.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_my_function(void) {
    // 测试代码
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_my_function);
    return UNITY_END();
}
```

### 步骤 3: 注册测试

编辑 `tests/CMakeLists.txt`:

```cmake
set(TESTS
    test_error
    test_memory
    test_my_module  # 添加这一行
)
```

### 步骤 4: 重新构建

```bash
cd build
cmake ..
make
ctest
```

---

## Unity 断言宏

Unity 提供以下常用断言:

### 布尔断言

```c
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);
```

### 相等性断言

```c
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_INT(expected, actual);
TEST_ASSERT_EQUAL_STRING(expected, actual);
TEST_ASSERT_EQUAL_PTR(expected, actual);
```

### NULL 断言

```c
TEST_ASSERT_NULL(ptr);
TEST_ASSERT_NOT_NULL(ptr);
```

### 比较断言

```c
TEST_ASSERT_GREATER_THAN(threshold, actual);
TEST_ASSERT_LESS_THAN(threshold, actual);
```

### 失败断言

```c
TEST_FAIL();
TEST_FAIL_MESSAGE("Custom error message");
```

---

## 测试最佳实践

### 1. 测试命名

- 测试函数命名: `test_<module>_<scenario>`
- 例如: `test_error_create_with_null_message`

### 2. 测试组织

- 每个模块一个测试文件
- 相关测试归类到一起
- 使用注释分隔测试组

### 3. 测试独立性

- 每个测试应该独立运行
- 使用 `setUp()` 和 `tearDown()` 管理状态
- 不依赖测试执行顺序

### 4. 资源清理

- 测试中分配的资源必须释放
- 使用 Valgrind 验证无内存泄漏

### 5. 错误情况测试

- 测试正常路径和错误路径
- 测试边界条件
- 测试 NULL 参数处理

---

## 测试覆盖率目标

CEL-C 项目的测试覆盖率目标:

- **Phase 1**: > 80%
- **Phase 2-4**: > 90%
- **Phase 5**: > 95%

### 查看覆盖率

```bash
cd build
make coverage
lcov --list coverage.info
```

---

## CI/CD 集成

GitHub Actions 会在每次 Push 和 PR 时自动运行:

- ✅ 单元测试 (Ubuntu + macOS)
- ✅ Valgrind 内存检查
- ✅ 代码覆盖率分析
- ✅ ASan 内存错误检测

查看 `.github/workflows/ci.yml` 了解详情。

---

## 故障排查

### 测试失败

```bash
# 查看详细输出
ctest --output-on-failure --verbose

# 运行单个测试
./tests/test_error
```

### 内存泄漏

```bash
# 详细的 Valgrind 输出
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./tests/test_error
```

### 覆盖率过低

- 检查未覆盖的代码路径
- 添加错误分支测试
- 测试边界条件

---

## 参考资源

- [Unity 文档](https://github.com/ThrowTheSwitch/Unity)
- [Valgrind 手册](https://valgrind.org/docs/manual/)
- [LCOV 文档](http://ltp.sourceforge.net/coverage/lcov.php)
