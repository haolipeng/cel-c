# CEL-C 快速启动指南

本指南帮助新加入的工程师快速了解项目状态和开始工作。

---

## 📊 当前项目状态

**当前阶段**: Phase 1 - 基础设施层

### 已完成的任务
- ✅ **Task 1.1: 项目结构与构建系统** (2026-01-05)
  - 完整的 CMake 构建系统
  - 第三方库自动下载
  - CI/CD 配置
  - 测试脚本

### 进行中的任务
- ⏳ **Task 1.2: 错误处理模块** (核心库工程师 A)
- ⏳ **Task 1.3: 内存管理模块** (核心库工程师 B)
- ⏳ **Task 1.4: 测试框架集成** (测试工程师)

这 3 个任务可以**完全并行**开发！

---

## 🚀 5 分钟快速开始

### 1. 克隆项目 (如果还未克隆)
```bash
cd /home/work
# 项目已创建在 /home/work/cel-c
```

### 2. 查看项目结构
```bash
cd /home/work/cel-c
tree -L 2
```

### 3. 阅读关键文档
```bash
# 项目 README
cat README.md

# Task 1.1 完成报告
cat TASK-1.1-COMPLETED.md

# Phase 1 实施指南
cat /home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md
```

### 4. 选择你的任务

#### 如果你是核心库工程师 A
**开始 Task 1.2: 错误处理模块**

参考文档: `/home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md` (Task 1.2 部分)

步骤:
1. 阅读 `cel_error.h` 接口设计
2. 复制代码模板到 `include/cel/cel_error.h`
3. 实现 `src/cel_error.c`
4. 编写 `tests/test_error.c`
5. 运行测试验证

预计时间: 2-3 天

#### 如果你是核心库工程师 B
**开始 Task 1.3: 内存管理模块**

参考文档: `/home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md` (Task 1.3 部分)

步骤:
1. 阅读 Arena 分配器设计
2. 实现 `include/cel/cel_memory.h`
3. 实现 `src/cel_memory.c`
4. 编写 `tests/test_memory.c`
5. 运行性能基准测试 (可选)

预计时间: 3-4 天

#### 如果你是测试工程师
**开始 Task 1.4: 测试框架集成**

参考文档: `/home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md` (Task 1.4 部分)

步骤:
1. 验证 Unity 集成
2. 创建测试辅助宏 `tests/test_helpers.h`
3. 配置测试 CMakeLists.txt
4. 验证测试脚本工作

预计时间: 2 天

---

## 📁 项目文件导航

### 核心文档
```
/home/work/cel-rust/specs/
├── README.md                            # 文档索引
├── product-requirements.md              # 产品需求 (功能列表)
├── c-design-01-overview-architecture.md # C 设计: 架构
├── c-design-02-data-structures.md       # C 设计: 数据结构
├── c-design-03-algorithms-api.md        # C 设计: 算法与 API
├── TASK-BREAKDOWN.md                    # 任务拆解 (38 个任务)
├── TASK-DEPENDENCIES.md                 # 任务依赖分析
└── PHASE-1-IMPLEMENTATION-GUIDE.md      # Phase 1 实施指南 ⭐
```

### 项目文件
```
/home/work/cel-c/
├── CMakeLists.txt                       # 主构建配置
├── README.md                            # 项目 README
├── TASK-1.1-COMPLETED.md               # Task 1.1 完成报告
├── include/cel/                         # 公共头文件 (待创建)
├── src/                                 # 实现文件 (占位符)
├── tests/                               # 单元测试 (待编写)
├── scripts/
│   ├── run_tests.sh                    # 测试运行脚本
│   └── run_memcheck.sh                 # 内存检查脚本
└── third_party/                         # 第三方库 (自动下载)
```

---

## 🛠️ 开发工具链

### 必需工具
- **GCC 13.3.0** ✅ (已安装)
- **Make** ✅ (已安装)
- **CMake 3.15+** ❌ (需要安装)
- **Git** (用于克隆第三方库)

### 可选工具
- **Valgrind**: 内存泄漏检测
- **LCOV**: 测试覆盖率报告
- **PCRE2**: 正则表达式支持

### 安装 CMake (如需本地构建)
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake

# 验证
cmake --version
```

---

## 📝 开发工作流

### 1. 实现功能
```bash
# 编辑头文件
vim include/cel/cel_error.h

# 编辑实现文件
vim src/cel_error.c
```

### 2. 编写测试
```bash
# 编辑测试文件
vim tests/test_error.c
```

### 3. 构建和测试
```bash
# 使用便捷脚本
./scripts/run_tests.sh

# 或手动构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
ctest --output-on-failure
```

### 4. 内存检查
```bash
./scripts/run_memcheck.sh

# 或手动运行
valgrind --leak-check=full ./build/tests/test_error
```

### 5. 代码审查
- 提交 Pull Request
- CI 会自动运行测试

---

## 📚 学习资源

### CEL 规范
- [Google CEL Spec](https://github.com/google/cel-spec)
- [cel-rust 文档](https://docs.rs/cel-interpreter/)

### C 语言资源
- [uthash 文档](https://troydhanson.github.io/uthash/)
- [SDS GitHub](https://github.com/antirez/sds)
- [Unity 测试框架](https://github.com/ThrowTheSwitch/Unity)

### 项目设计文档
参考 `/home/work/cel-rust/specs/` 目录下的完整设计文档

---

## 🎯 Phase 1 目标

### Milestone: 基础设施完成
**预计时间**: Week 1-2

**目标**:
- [x] Task 1.1: 构建系统 ✅
- [ ] Task 1.2: 错误处理 (2-3 天)
- [ ] Task 1.3: 内存管理 (3-4 天)
- [ ] Task 1.4: 测试框架 (2 天)

**完成标准**:
- ✅ 构建系统可编译
- ✅ 错误处理测试 100% 通过
- ✅ Arena 分配器测试通过
- ✅ 所有测试无内存泄漏

---

## 💬 常见问题

### Q: 我应该从哪个任务开始?
**A**: 根据你的角色:
- 核心库工程师 A → Task 1.2 (错误处理)
- 核心库工程师 B → Task 1.3 (内存管理)
- 测试工程师 → Task 1.4 (测试框架)

### Q: 代码模板在哪里?
**A**: `/home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md` 包含完整的代码模板，可以直接复制使用。

### Q: Task 1.2 和 1.3 可以并行吗?
**A**: 可以！它们只依赖 Task 1.1，彼此之间无依赖。

### Q: 如何验证我的实现是否正确?
**A**:
1. 单元测试 100% 通过
2. Valgrind 无内存泄漏
3. 代码审查通过

### Q: Phase 1 完成后做什么?
**A**: 开始 Phase 2 - 核心数据结构:
- Task 2.1: 基础值类型 (依赖 Task 1.2, 1.3)
- Task 2.2-2.6: 其他数据类型

---

## 🔗 相关链接

- **Task 拆解**: `/home/work/cel-rust/specs/TASK-BREAKDOWN.md`
- **依赖分析**: `/home/work/cel-rust/specs/TASK-DEPENDENCIES.md`
- **实施指南**: `/home/work/cel-rust/specs/PHASE-1-IMPLEMENTATION-GUIDE.md`
- **设计文档**: `/home/work/cel-rust/specs/c-design-*.md`

---

## ✅ 检查清单

新工程师入职清单:
- [ ] 阅读项目 README
- [ ] 阅读 Phase 1 实施指南
- [ ] 了解自己负责的任务
- [ ] 安装必要的开发工具
- [ ] 克隆并浏览代码
- [ ] 运行第一个测试 (Task 1.1 完成后)
- [ ] 开始实现第一个模块

---

**祝开发顺利！如有问题，请参考设计文档或咨询项目负责人。** 🚀
