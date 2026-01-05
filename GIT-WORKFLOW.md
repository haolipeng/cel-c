# Git 工作流指南

本文档说明 CEL-C 项目的 Git 工作流和 Issue 管理。

---

## 📋 Commit 消息规范

### 格式

```
<type>: <subject> (Task X.Y)

<body>

Closes #<issue-number>

🤖 Generated with Claude Code

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

### Type 类型

- **feat**: 新功能
- **fix**: Bug 修复
- **docs**: 文档更新
- **style**: 代码格式 (不影响功能)
- **refactor**: 重构 (不是新功能也不是 bug 修复)
- **test**: 测试相关
- **chore**: 构建工具、依赖更新等

### 示例

```
feat: 实现错误处理模块 (Task 1.2)

实现 cel_error.h 和 cel_error.c，提供统一的错误处理机制。

## 主要变更

- 定义 16 种错误码
- 实现 cel_error_t 和 cel_result_t 结构
- 实现错误传播宏 CEL_TRY 和 CEL_UNWRAP
- 添加完整的单元测试

## 测试结果

- 单元测试: 7/7 通过 ✅
- Valgrind: 无内存泄漏 ✅
- 测试覆盖率: 95%

Closes #2

🤖 Generated with Claude Code

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

---

## 🔗 Issue 关联

### 在 Commit 中关联 Issue

使用以下关键词自动关闭 Issue:

- `Closes #123`
- `Fixes #123`
- `Resolves #123`

### Issue 编号规则

- **#1**: Task 1.1 - 项目结构 (已完成 ✅)
- **#2**: Task 1.2 - 错误处理
- **#3**: Task 1.3 - 内存管理
- **#4**: Task 1.4 - 测试框架
- **#5+**: 后续任务

### 在 PR 中关联 Issue

Pull Request 标题格式:
```
[Task X.Y] <简短描述>
```

PR 描述中关联 Issue:
```markdown
## 相关 Issue

Closes #2

## 变更说明

...

## 测试结果

...
```

---

## 🌿 分支策略

### 主分支

- **master** (或 **main**): 主分支，始终保持可构建
- **develop**: 开发分支 (可选)

### 功能分支

每个任务创建独立分支:

```bash
# 创建功能分支
git checkout -b task/1.2-error-handling

# 完成开发后
git add .
git commit -m "feat: 实现错误处理模块 (Task 1.2)

...

Closes #2
"

# 推送到远程
git push origin task/1.2-error-handling
```

### 分支命名规范

```
task/<任务编号>-<简短描述>
```

示例:
- `task/1.2-error-handling`
- `task/1.3-memory-management`
- `task/2.1-value-types`

---

## 🚀 完整工作流

### 1. 认领 Issue

在 GitHub 上认领 Issue:
1. 进入 Issue 页面
2. 评论 "I'll take this"
3. 将 Issue 分配给自己
4. 将 Issue 标签改为 "in-progress"

### 2. 创建功能分支

```bash
cd /home/work/cel-c

# 更新主分支
git checkout master
git pull origin master

# 创建功能分支
git checkout -b task/1.2-error-handling
```

### 3. 开发和测试

```bash
# 编辑代码
vim include/cel/cel_error.h
vim src/cel_error.c
vim tests/test_error.c

# 运行测试
./scripts/run_tests.sh

# 内存检查
./scripts/run_memcheck.sh
```

### 4. 提交代码

```bash
# 查看修改
git status
git diff

# 暂存文件
git add include/cel/cel_error.h
git add src/cel_error.c
git add tests/test_error.c

# 提交 (使用规范的 commit 消息)
git commit -m "feat: 实现错误处理模块 (Task 1.2)

实现 cel_error.h 和 cel_error.c，提供统一的错误处理机制。

## 主要变更

- 定义 16 种错误码
- 实现 cel_error_t 和 cel_result_t 结构
- 实现错误传播宏 CEL_TRY 和 CEL_UNWRAP
- 添加完整的单元测试

## 测试结果

- 单元测试: 7/7 通过 ✅
- Valgrind: 无内存泄漏 ✅
- 测试覆盖率: 95%

Closes #2

🤖 Generated with Claude Code

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
"
```

### 5. 推送到远程

```bash
# 首次推送
git push -u origin task/1.2-error-handling

# 后续推送
git push
```

### 6. 创建 Pull Request

1. 访问 GitHub 仓库
2. 点击 "Compare & pull request"
3. 填写 PR 标题和描述:

```markdown
[Task 1.2] 实现错误处理模块

## 相关 Issue

Closes #2

## 变更说明

实现了完整的错误处理模块:
- 16 种错误码定义
- cel_error_t 和 cel_result_t 结构
- 错误传播宏
- 7 个单元测试

## 测试结果

- ✅ 单元测试全部通过
- ✅ Valgrind 无内存泄漏
- ✅ 测试覆盖率 95%
- ✅ CI 构建通过

## 验收清单

- [x] 功能完整实现
- [x] 单元测试通过
- [x] 无内存泄漏
- [x] 代码审查通过
- [x] 文档更新
```

4. 请求代码审查
5. CI 自动运行测试
6. 合并到 master

### 7. 合并后清理

```bash
# 切换回主分支
git checkout master
git pull origin master

# 删除本地功能分支
git branch -d task/1.2-error-handling

# 删除远程分支 (GitHub 可以自动删除)
git push origin --delete task/1.2-error-handling
```

---

## 📝 Commit 消息模板

创建 commit 消息模板:

**文件**: `.gitmessage`

```
<type>: <subject> (Task X.Y)

# 详细描述变更内容
#
# ## 主要变更
# -
# -
#
# ## 测试结果
# -
#
# Closes #<issue-number>
#
# 🤖 Generated with Claude Code
#
# Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

配置 Git 使用模板:

```bash
git config --local commit.template .gitmessage
```

---

## 🔍 常用 Git 命令

### 查看状态和历史

```bash
# 查看当前状态
git status

# 查看提交历史
git log --oneline --graph --decorate

# 查看某个文件的历史
git log --follow -- src/cel_error.c

# 查看某次提交的详情
git show HEAD
git show 54aa0c4
```

### 查看差异

```bash
# 查看未暂存的修改
git diff

# 查看已暂存的修改
git diff --staged

# 查看两个提交之间的差异
git diff master..task/1.2-error-handling
```

### 分支操作

```bash
# 列出所有分支
git branch -a

# 创建并切换分支
git checkout -b task/1.3-memory

# 切换分支
git checkout master

# 删除分支
git branch -d task/1.2-error-handling
```

### 撤销操作

```bash
# 撤销未暂存的修改
git checkout -- src/cel_error.c

# 撤销已暂存的修改
git reset HEAD src/cel_error.c

# 修改最后一次提交 (未推送时)
git commit --amend

# 回退到上一个提交 (保留修改)
git reset --soft HEAD^

# 完全回退到上一个提交 (丢弃修改)
git reset --hard HEAD^
```

---

## 📊 当前 Commit 记录

### master 分支

```
54aa0c4 (HEAD -> master) feat: 完成项目基础设施搭建 (Task 1.1)
```

**统计**:
- 34 个文件修改
- 1,765 行新增
- Closes #1

---

## 🎯 下一步

### 待创建的分支

1. `task/1.2-error-handling` - 错误处理模块
2. `task/1.3-memory-management` - 内存管理模块
3. `task/1.4-testing-framework` - 测试框架

### 待提交的 Issues

在 GitHub 上创建 Issues:
1. Task 1.2: 错误处理 (使用 `.github/ISSUE_TEMPLATE/task-1.2-error-handling.md`)
2. Task 1.3: 内存管理 (使用 `.github/ISSUE_TEMPLATE/task-1.3-memory-management.md`)
3. Task 1.4: 测试框架 (使用 `.github/ISSUE_TEMPLATE/task-1.4-testing-framework.md`)

---

## 💡 最佳实践

1. **小而频繁的提交** - 每个功能点一个 commit
2. **描述性的消息** - 说明"为什么"而不仅仅是"做了什么"
3. **关联 Issue** - 使用 `Closes #X` 自动关闭 Issue
4. **运行测试** - 提交前确保测试通过
5. **代码审查** - 所有代码通过 PR 合并
6. **保持主分支稳定** - 主分支始终可构建和测试通过

---

## 📚 参考资源

- [Conventional Commits](https://www.conventionalcommits.org/)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
- [Git 文档](https://git-scm.com/doc)
- [GitHub Issues 文档](https://docs.github.com/en/issues)
