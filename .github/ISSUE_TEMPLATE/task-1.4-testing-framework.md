---
name: Task 1.4 - 测试框架集成
about: 完善测试框架配置和工具
title: "[Task 1.4] 完善测试框架集成"
labels: enhancement, phase-1, testing, P1
assignees: ''
---

## 📋 任务描述

完善测试框架集成，提供便捷的测试辅助宏和工具，确保测试基础设施完整。

## 🎯 任务目标

- 创建测试辅助宏库
- 完善测试 CMake 配置
- 验证所有测试脚本正常工作
- 确保覆盖率和内存检查工具可用

## 📦 交付物

- [ ] `tests/test_helpers.h` - 测试辅助宏
- [ ] 更新 `tests/CMakeLists.txt` - 完善配置
- [ ] 验证 `scripts/run_tests.sh` - 测试运行脚本
- [ ] 验证 `scripts/run_memcheck.sh` - Valgrind 脚本
- [ ] 更新文档说明测试流程

## 🔗 依赖关系

**依赖**:
- #1 Task 1.1: 项目结构与构建系统 ✅

**可选依赖**:
- #2 Task 1.2: 错误处理 (用于测试示例)
- #3 Task 1.3: 内存管理 (用于测试示例)

**被依赖**:
- 所有后续测试任务都会使用此框架

## ✅ 验收标准

- [ ] Unity 测试框架正确集成
- [ ] `test_helpers.h` 提供便捷的测试宏
- [ ] CMakeLists.txt 自动注册所有测试
- [ ] `ctest` 可以运行所有测试
- [ ] `run_tests.sh` 脚本工作正常
- [ ] `run_memcheck.sh` 可以运行 Valgrind
- [ ] 覆盖率目标 `make coverage` 可用 (Debug 模式)
- [ ] GitHub Actions CI 正常工作

## 📚 参考文档

- [Phase 1 实施指南](../specs/PHASE-1-IMPLEMENTATION-GUIDE.md) - Task 1.4 部分
- [Unity 测试框架文档](https://github.com/ThrowTheSwitch/Unity)

## ⏱️ 预计工时

**2 天**

## 👤 建议负责人

测试工程师

## 📝 实施步骤

### 第 1 步: 创建测试辅助宏 (1 小时)
```c
// 在 tests/test_helpers.h 中定义:
// - TEST_ASSERT_STRING_EQUAL_OR_NULL
// - TEST_ASSERT_DOUBLE_APPROX
// - TEST_ASSERT_MEMORY_EQUAL
// - 其他便捷宏
```

### 第 2 步: 完善 CMake 配置 (1 小时)
```cmake
// 在 tests/CMakeLists.txt 中:
// - 自动发现测试文件
// - 配置覆盖率目标
// - 配置 Valgrind 目标
// - 设置测试超时
```

### 第 3 步: 验证测试脚本 (2 小时)
```bash
# 测试 run_tests.sh
./scripts/run_tests.sh

# 测试 run_memcheck.sh
./scripts/run_memcheck.sh

# 确保脚本在各种情况下都能工作
```

### 第 4 步: 编写测试文档 (1 小时)
- 更新 README.md 测试部分
- 创建 TESTING.md 文档
- 说明如何添加新测试

### 第 5 步: 验证 CI/CD (1 小时)
- 推送到 GitHub 触发 CI
- 验证 Ubuntu 和 macOS 构建
- 验证内存检查和覆盖率

## 🔍 验证要点

- ✅ Unity 自动下载和编译
- ✅ 测试辅助宏简化测试编写
- ✅ `ctest` 可以发现并运行所有测试
- ✅ 测试失败时有清晰的错误信息
- ✅ Valgrind 检查所有测试
- ✅ 覆盖率报告生成正确
- ✅ CI 在 PR 时自动运行

## 💡 提示

### 测试辅助宏示例
```c
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
```

### CMake 测试自动注册
```cmake
foreach(test_name ${TESTS})
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} PRIVATE cel_static unity)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
```

### 添加新测试的步骤
1. 在 `tests/` 创建 `test_xxx.c`
2. 在 `tests/CMakeLists.txt` 的 TESTS 列表添加 `test_xxx`
3. 重新运行 `cmake`

## 📊 测试覆盖率目标

- **Phase 1**: > 80%
- **Phase 2-4**: > 90%
- **Phase 5**: > 95%

## 🎯 交付标准

完成后应该能够:
```bash
# 一键运行所有测试
./scripts/run_tests.sh

# 一键内存检查
./scripts/run_memcheck.sh

# 生成覆盖率报告
cd build
make coverage

# 查看测试列表
ctest -N
```

## 相关 Issues

- #1 Task 1.1: 项目结构
- #2 Task 1.2: 错误处理 (将使用此框架)
- #3 Task 1.3: 内存管理 (将使用此框架)

## 相关链接

- Task 拆解: [TASK-BREAKDOWN.md](../specs/TASK-BREAKDOWN.md)
- 任务依赖: [TASK-DEPENDENCIES.md](../specs/TASK-DEPENDENCIES.md)
