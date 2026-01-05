---
name: Task 1.3 - 内存管理模块
about: 实现 CEL-C 的内存管理模块 (Arena 分配器)
title: "[Task 1.3] 实现内存管理模块 (Arena 分配器)"
labels: enhancement, phase-1, P0
assignees: ''
---

## 📋 任务描述

实现 CEL-C 的 Arena 内存分配器，用于快速分配 AST 节点和其他临时对象。

## 🎯 任务目标

- 实现 Arena 分配器 (支持多块链接、对齐、重置)
- 提供引用计数辅助宏 (单线程和多线程版本)
- 编写完整的单元测试和性能基准测试

## 📦 交付物

- [ ] `include/cel/cel_memory.h` - Arena 分配器接口
- [ ] `src/cel_memory.c` - Arena 实现
- [ ] `tests/test_memory.c` - 单元测试 (7+ 测试用例)
- [ ] `bench/bench_arena.c` - 性能基准测试 (可选)

## 🔗 依赖关系

**依赖**:
- #1 Task 1.1: 项目结构与构建系统 ✅

**被依赖**:
- Task 2.1: 基础值类型 (引用计数)
- Task 2.5: AST 节点 (Arena 分配)
- Task 4.1: 执行上下文 (可选使用 Arena)

## ✅ 验收标准

- [ ] Arena 分配器正确实现
- [ ] 支持多个内存块自动链接
- [ ] 内存对齐处理正确 (8 字节对齐)
- [ ] `arena_reset()` 可以重用内存
- [ ] 单元测试 100% 通过
- [ ] Valgrind 检查无内存泄漏
- [ ] ARENA_ALLOC 宏正常工作
- [ ] (可选) Arena 比 malloc 快 2-5 倍

## 📚 参考文档

- [Phase 1 实施指南](../specs/PHASE-1-IMPLEMENTATION-GUIDE.md) - Task 1.3 部分
- [C 设计文档 - AST](../specs/c-design-02-data-structures.md) - 3.2.2 AST 内存管理
- [C 设计文档 - 内存策略](../specs/c-design-03-algorithms-api.md) - 5.3 内存管理策略

## ⏱️ 预计工时

**3-4 天**

## 👤 建议负责人

核心库工程师 B

## 📝 实施步骤

### 第 1 步: 定义 Arena 接口 (1 小时)
```c
// 在 include/cel/cel_memory.h 中定义:
// - arena_t 类型 (不透明指针)
// - arena_create/alloc/reset/destroy API
// - ARENA_ALLOC 宏
// - 引用计数宏 (单线程/多线程)
```

### 第 2 步: 实现 Arena 核心逻辑 (4-5 小时)
```c
// 在 src/cel_memory.c 中实现:
// - arena_block_t 内部结构
// - 内存块链接管理
// - 对齐计算
// - 自动扩容
```

### 第 3 步: 编写单元测试 (2-3 小时)
```c
// 在 tests/test_memory.c 中测试:
// - 创建和销毁
// - 简单分配
// - 大量分配 (触发多块)
// - 重置功能
// - 对齐正确性
// - 宏功能
```

### 第 4 步: 性能基准测试 (可选, 1-2 小时)
```c
// 在 bench/bench_arena.c 中对比:
// - Arena vs malloc 性能
// - 100 万次分配测试
```

### 第 5 步: 验证和优化 (1 小时)
- 运行所有测试
- Valgrind 内存检查
- 性能分析

## 🔍 测试要点

- ✅ Arena 创建和销毁正确
- ✅ 内存分配正确对齐
- ✅ 多次分配触发多个块
- ✅ reset 后可以重用内存
- ✅ 零分配返回 NULL
- ✅ 无内存泄漏
- ✅ 宏展开正确

## 💡 提示

- 使用链表管理多个内存块
- 默认块大小 4KB
- 对齐使用位运算: `(offset + alignment - 1) & ~(alignment - 1)`
- Arena 销毁时一次性释放所有块
- 线程安全版本使用 `stdatomic.h` 中的 atomic_int

## 🎯 性能目标

- Arena 分配比 malloc 快 **2-5 倍**
- 100 万次分配 < 0.5 秒

## 相关链接

- Task 拆解: [TASK-BREAKDOWN.md](../specs/TASK-BREAKDOWN.md)
- 任务依赖: [TASK-DEPENDENCIES.md](../specs/TASK-DEPENDENCIES.md)
