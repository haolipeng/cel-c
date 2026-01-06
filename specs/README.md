# CEL-Rust 移植到 C 语言 - 文档索引

本目录包含将 CEL-Rust 项目从 Rust 移植到 C 语言所需的完整技术文档。

---

## 文档概览

### 📋 产品需求文档
**文件**: `product-requirements.md`
**行数**: 1,590 行
**用途**: 详细列出 CEL-Rust 的所有功能特性,作为 C 语言移植的功能参考

**主要内容:**
1. 项目概述与应用场景
2. 核心功能特性(运算符、数据类型、内置函数、宏系统)
3. 解析器规范(词法规则、语法规则、宏展开)
4. 执行模型(编译-执行两阶段、AST 求值、短路求值)
5. 扩展机制(函数注册、自定义类型、变量解析器)
6. 性能与安全要求
7. 使用示例与特性标志

### 🏗️ C 语言设计文档

设计文档分为三个部分,共 3,013 行:

#### 第 1 部分: 概述与架构
**文件**: `c-design-01-overview-architecture.md`
**行数**: 631 行

**主要内容:**
- **设计概述**: C 语言实现的设计目标与原则
- **与 Rust 实现的差异**: 内存管理、错误处理、类型系统等方面的对比
- **技术选型**: C11 标准,第三方库选择(uthash, SDS, PCRE2, cJSON 等)
- **整体架构**: 模块划分、依赖关系图、编译/执行流程图
- **目录结构**: 推荐的项目文件组织
- **构建系统**: CMake 配置建议

**关键架构图:**
- 系统架构图 (Mermaid)
- 模块依赖关系图 (Mermaid)
- 编译流程图 (Mermaid Sequence)
- 执行流程图 (Mermaid Sequence)

#### 第 2 部分: 核心数据结构
**文件**: `c-design-02-data-structures.md`
**行数**: 979 行

**主要内容:**
- **cel_value**: 值类型系统设计
  - Tagged union 实现
  - 引用计数内存管理
  - 字符串、字节数组、列表、Map 结构
  - Optional 和 Opaque 类型

- **cel_ast**: AST 节点表示
  - 所有 AST 节点类型定义
  - Arena 分配器优化

- **cel_context**: 执行上下文
  - 变量表和函数注册表(基于 uthash)
  - 作用域链(父子上下文)
  - 变量解析器接口

- **cel_functions**: 函数接口
  - 函数指针类型定义
  - 函数元数据结构
  - 内置函数实现模式

- **cel_error**: 错误处理
  - 错误码枚举
  - Result-like 模式
  - 错误传播宏

**完整的 C 结构体定义,可直接用于实现。**

#### 第 3 部分: 算法、API 与实现
**文件**: `c-design-03-algorithms-api.md`
**行数**: 1,403 行

**主要内容:**
- **解析流程设计**:
  - 推荐工具链(re2c + Lemon)
  - 词法分析器示例(re2c 语法)
  - 语法分析器示例(Lemon 语法)
  - 宏展开算法(has, all, exists, map, filter)

- **求值算法设计**:
  - 核心求值函数实现
  - 短路求值优化(&&, ||)
  - 懒惰求值(三元运算符)
  - 推导式(Comprehension)执行

- **内存管理策略**:
  - 引用计数的循环引用问题与解决方案
  - Arena 分配器实现
  - Copy-on-Write 优化

- **并发安全设计**:
  - 不可变值共享
  - 线程安全的引用计数(atomic 操作)

- **公开 API 设计**:
  - 完整的 C API 头文件
  - 使用示例(基本用法、变量、自定义函数)

- **实现建议**:
  - 开发阶段划分(MVP → 完整实现,共 5 个阶段)
  - 测试策略(单元测试、集成测试、模糊测试)
  - 性能基准测试
  - 与 Rust 版本的兼容性验证
  - 代码风格规范(Linux Kernel Style)

---

## 文档使用指南

### 对于项目评估者
1. 先阅读 `product-requirements.md` 了解 CEL 的功能
2. 浏览 `c-design-01-overview-architecture.md` 了解整体架构

### 对于架构设计者
1. 精读三份 C 设计文档
2. 重点关注架构图和模块划分
3. 评估第三方库选型是否合适

### 对于实现开发者
1. 使用 `product-requirements.md` 作为功能清单
2. 按照 `c-design-02-data-structures.md` 实现核心数据结构
3. 参考 `c-design-03-algorithms-api.md` 实现算法和 API
4. 遵循文档中的开发阶段划分,从 MVP 开始

### 对于测试工程师
1. 从 `product-requirements.md` 提取功能测试用例
2. 使用 `c-design-03-algorithms-api.md` 中的测试策略

---

## 文档统计

| 文档 | 行数 | 大小 | 主要内容 |
|------|------|------|----------|
| 产品需求文档 | 1,590 | 37K | 功能规范 |
| C 设计 - 第 1 部分 | 631 | 19K | 概述与架构 |
| C 设计 - 第 2 部分 | 979 | 27K | 核心数据结构 |
| C 设计 - 第 3 部分 | 1,403 | 36K | 算法与 API |
| **总计** | **4,603** | **119K** | **完整移植指南** |

---

## 技术栈总结

### Rust 实现 (源项目)
- 语言: Rust (2021 edition)
- 解析器: ANTLR4 (antlr4rust)
- 内存管理: Arc (原子引用计数)
- 并发: Send + Sync traits
- 错误处理: Result<T, E>

### C 实现 (目标设计)
- 语言: C11/C17
- 解析器: re2c (词法) + Lemon (语法)
- 内存管理: 引用计数 + Arena 分配器
- 并发: atomic_int (stdatomic.h)
- 错误处理: Result-like struct
- 哈希表: uthash (Header-only)
- 字符串: SDS (Simple Dynamic Strings)
- 正则: PCRE2
- JSON: cJSON
- 测试: Unity
- 构建: CMake

---

## 关键设计决策

1. **C 标准选择**: C11 (广泛支持,提供 atomic 操作)
2. **解析器工具**: re2c + Lemon (高性能,无运行时依赖)
3. **内存管理**: 引用计数 + Arena (平衡性能和简单性)
4. **容器选择**: uthash (成熟稳定,零依赖)
5. **API 风格**: 传统 C 风格(指针、返回码、输出参数)
6. **并发模型**: 不可变值共享 + 线程局部 Context
7. **开发策略**: MVP 迭代(2-3 个月基础实现 → 6-9 个月完整实现)

---

## 下一步行动

1. **环境搭建**: 安装 C11 编译器、CMake、re2c、Lemon
2. **MVP 开发**: 从 cel_value 和 cel_ast 开始实现
3. **单元测试**: 每个模块完成后编写测试
4. **性能测试**: 与 Rust 版本对比
5. **文档完善**: 补充 API 文档和示例

---

## 相关资源

- [CEL 官方规范](https://github.com/google/cel-spec)
- [cel-rust GitHub](https://github.com/cel-rust/cel-rust)
- [re2c 文档](https://re2c.org/)
- [Lemon 解析器](https://www.sqlite.org/lemon.html)
- [uthash 文档](https://troydhanson.github.io/uthash/)
- [SDS GitHub](https://github.com/antirez/sds)

---

## 文档版本

- 生成日期: 2026-01-03
- 基于版本: cel-rust 0.12.0
- 文档作者: Claude (Anthropic)
- 目标读者: C 语言移植开发团队

---

**祝移植项目顺利! 🚀**
