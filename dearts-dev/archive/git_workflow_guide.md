# DearTs Framework - 企业级 Git 工作流程指南

> 本文档定义了 DearTs Framework 项目的标准 Git 工作流程，确保团队协作高效、代码质量可控、版本发布稳定。

---

## 目录

1. [分支模型](#1-分支模型)
2. [分支命名规范](#2-分支命名规范)
3. [工作流程](#3-工作流程)
4. [提交规范](#4-提交规范)
5. [代码审查](#5-代码审查)
6. [发布流程](#6-发布流程)
7. [紧急修复](#7-紧急修复)
8. [常用命令](#8-常用命令)

---

## 1. 分支模型

DearTs Framework 采用 **Git Flow** 分支策略，适合企业级项目开发和版本管理。

### 主要分支

```
main (生产环境)
  ↑
  │ merge
  │
develop (开发环境)
  ↑
  │ merge
  │
feature/* (功能分支)
hotfix/* (紧急修复)
release/* (发布准备)
```

#### 1.1 `main` 分支

- **用途**: 生产环境代码，始终保持稳定可发布状态
- **保护规则**:
  - 禁止直接推送
  - 必须通过 Pull Request 合并
  - 需要至少 1 个审查批准
  - 必须通过 CI 检查
- **标签**: 每次合并到 main 打上版本标签（如 `v1.0.0`）

#### 1.2 `develop` 分支

- **用途**: 开发主分支，集成所有功能开发
- **状态**: 相对于 main 可能不稳定，但需要保持可编译
- **保护规则**:
  - 禁止直接推送
  - 必须通过 Pull Request 合并
  - 需要至少 1 个审查批准

#### 1.3 `feature/*` 分支

- **用途**: 开发新功能
- **基于**: `develop`
- **合并回**: `develop`
- **命名**: `feature/<功能名称>-<简短描述>`
- **生命周期**: 功能完成后删除

#### 1.4 `hotfix/*` 分支

- **用途**: 紧急修复生产环境问题
- **基于**: `main` (或已发布标签)
- **合并回**: `main` **和** `develop`
- **命名**: `hotfix/<问题编号>-<问题描述>`
- **生命周期**: 修复完成后删除

#### 1.5 `release/*` 分支

- **用途**: 发布准备（测试、bug 修复、文档更新）
- **基于**: `develop`
- **合并回**: `main` **和** `develop`
- **命名**: `release/<版本号>`
- **生命周期**: 发布完成后删除

### 分支关系图

```
           ┌─────────────┐
           │    main     │ ← 生产环境 (稳定)
           └──────┬──────┘
                  │ merge
         ┌────────┴────────┐
         │                 │
    ┌────┴────┐      ┌─────┴────┐
    │ release │      │  hotfix  │ ← 紧急修复
    └────┬────┘      └─────┬────┘
         │ merge            │ merge
         └──┬──┬────────────┘
            │  │
      ┌─────┴──┴──────┐
      │   develop     │ ← 开发环境
      └───────┬───────┘
              │ merge
      ┌───────┴────────┐
      │                │
┌─────┴─────┐   ┌─────┴─────┐
│ feature/1 │   │ feature/2 │ ... 功能开发
└───────────┘   └───────────┘
```

---

## 2. 分支命名规范

### 2.1 功能分支

```
feature/<ticket-id>-<short-description>

示例:
feature/GIT-101-add-plugin-manager
feature/GIT-102-implement-event-bus
feature/GIT-103-refactor-config-system
```

**命名规则**:
- 前缀: `feature/`
- 票号: `<项目缩写>-<编号>` (如 GIT-101, 若无票号可省略)
- 描述: 小写字母、连字符分隔、不超过 50 字符
- **禁止**: 使用中文、特殊字符、下划线

### 2.2 修复分支

**Bug 修复 (非紧急)**:
```
bugfix/<ticket-id>-<short-description>

示例:
bugfix/GIT-201-fix-memory-leak
bugfix/GIT-202-resolve-crash-on-startup
```

**紧急修复**:
```
hotfix/<ticket-id>-<short-description>

示例:
hotfix/GIT-301-critical-security-fix
hotfix/GIT-302-production-crash-hotfix
```

### 2.3 发布分支

```
release/<major>.<minor>.<patch>

示例:
release/1.0.0
release/1.1.0
release/2.0.0
```

### 2.4 实验性分支

**用于实验性功能或技术探索**:
```
experiment/<feature-name>-<description>

示例:
experiment/new-rendering-pipeline
experiment/async-task-refactor
```

---

## 3. 工作流程

### 3.1 开始新功能开发

```bash
# 1. 确保本地 develop 是最新的
git checkout develop
git pull origin develop

# 2. 创建功能分支
git checkout -b feature/GIT-101-add-new-feature

# 3. 开始开发...
# 进行代码修改、添加功能等

# 4. 提交更改（遵循提交规范）
git add .
git commit -m "feat: add user authentication system"

# 5. 推送到远程
git push -u origin feature/GIT-101-add-new-feature

# 6. 创建 Pull Request
# 在 GitHub 上创建 PR: feature/GIT-101-add-new-feature -> develop
```

### 3.2 功能开发完成流程

```bash
# 1. 确保所有更改已提交
git status

# 2. 同步最新的 develop
git fetch origin develop
git rebase origin/develop

# 3. 解决冲突（如果有）
# 编辑冲突文件
git add <resolved-files>
git rebase --continue

# 4. 推送到远程（可能需要 force push）
git push --force-with-lease

# 5. 在 GitHub 上创建/更新 Pull Request
# - 填写 PR 模板
# - 关联相关 Issue
# - 请求代码审查

# 6. 等待 CI 检查通过和代码审查批准

# 7. 合并 PR (使用 Squash and Merge)
# - 保持 develop 历史清洁

# 8. 删除功能分支（本地和远程）
git checkout develop
git branch -d feature/GIT-101-add-new-feature
git push origin --delete feature/GIT-101-add-new-feature
```

### 3.3 紧急修复流程

**场景**: 生产环境（main 分支）出现严重 bug，需要立即修复

```bash
# 1. 从 main 创建 hotfix 分支
git checkout main
git pull origin main
git checkout -b hotfix/GIT-301-critical-security-fix

# 2. 修复问题
# 编辑代码、测试修复

# 3. 提交修复
git add .
git commit -m "hotfix: patch security vulnerability in authentication"

# 4. 推送到远程
git push -u origin hotfix/GIT-301-critical-security-fix

# 5. 创建 PR 到 main
# hotfix/GIT-301-critical-security-fix -> main

# 6. 审查通过后合并到 main

# 7. 打标签
git checkout main
git pull origin main
git tag -a v1.0.1 -m "Hotfix: patch security vulnerability"
git push origin v1.0.1

# 8. 合并回 develop（确保修复也应用到开发环境）
git checkout develop
git merge hotfix/GIT-301-critical-security-fix
git push origin develop

# 9. 删除 hotfix 分支
git branch -d hotfix/GIT-301-critical-security-fix
git push origin --delete hotfix/GIT-301-critical-security-fix
```

### 3.4 发布流程

```bash
# 1. 从 develop 创建 release 分支
git checkout develop
git pull origin develop
git checkout -b release/1.0.0

# 2. 更新版本号
# 编辑 VERSION 文件或 CMakeLists.txt

# 3. 进行发布前的修复和优化
# - 修复发现的 bug
# - 更新文档
# - 更新 CHANGELOG

git add .
git commit -m "chore: prepare for release v1.0.0"

# 4. 推送到远程
git push -u origin release/1.0.0

# 5. 测试验证
# - 运行完整测试套件
# - 手动测试关键功能
# - 性能测试

# 6. 合并到 main
git checkout main
git merge release/1.0.0
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin main
git push origin v1.0.0

# 7. 合并回 develop
git checkout develop
git merge release/1.0.0
git push origin develop

# 8. 删除 release 分支
git branch -d release/1.0.0
git push origin --delete release/1.0.0
```

---

## 4. 提交规范

DearTs Framework 采用 **Conventional Commits** 规范。

### 4.1 提交消息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 4.2 Type（类型）

| Type | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat(plugin): add plugin loading system` |
| `fix` | Bug 修复 | `fix(event): resolve event subscription crash` |
| `docs` | 文档更新 | `docs(api): update plugin system guide` |
| `style` | 代码风格（不影响功能） | `style: format code with clang-format` |
| `refactor` | 重构（不是新功能也不是修复） | `refactor(config): simplify config manager` |
| `perf` | 性能优化 | `perf(render): optimize texture loading` |
| `test` | 测试相关 | `test(event): add unit tests for event bus` |
| `chore` | 构建过程或辅助工具变动 | `chore: update CMakeLists.txt` |
| `ci` | CI 配置文件和脚本 | `ci: add GitHub Actions workflow` |
| `revert` | 回滚之前的提交 | `revert: feat(plugin system)` |

### 4.3 Scope（范围）

常用范围：
- `plugin` - 插件系统
- `event` - 事件系统
- `config` - 配置系统
- `task` - 任务系统
- `ui` - UI 组件
- `core` - 核心功能
- `docs` - 文档
- `build` - 构建系统
- `ci` - CI/CD

### 4.4 Subject（主题）

- 使用动词原形开头
- 小写字母
- 不要句号结尾
- 不超过 50 字符

**示例**:
```
feat(plugin): add plugin loading system
fix(event): resolve memory leak in event bus
docs(readme): update build instructions
```

### 4.5 Body（正文）

- 详细描述提交内容
- 说明 **what** 和 **why**（不是 how）
- 每行不超过 72 字符

**示例**:
```
feat(plugin): add plugin loading system

Implement dynamic plugin loading from directory with:
- Plugin discovery and validation
- Lifecycle management (load/enable/disable/unload)
- Error handling and logging

Closes #GIT-101
```

### 4.6 Footer（脚注）

**关联 Issue**:
```
Closes #GIT-101
Fixes #GIT-102
Refs #GIT-103
```

**破坏性变更**:
```
BREAKING CHANGE: Event bus API now requires explicit type registration
```

### 4.7 完整示例

```
feat(event): implement type-safe event bus

Add a compile-time type-safe event bus for decoupled communication:
- Template-based event subscription
- Automatic subscriber management
- RAII token for automatic unsubscription
- Thread-safe event publishing

This replaces the old string-based event system for better type safety.

Closes #GIT-102
```

### 4.8 提交最佳实践

**✅ DO**:
- 每个提交做一件事
- 提交前运行测试
- 保持提交历史清洁
- 使用有意义的消息
- 频繁提交（小步快跑）

**❌ DON'T**:
- 混合不相关的更改
- 提交测试失败或构建失败的代码
- 使用 "fix bug", "update" 等模糊消息
- 一次提交太多文件
- 提交敏感信息（密钥、密码）

### 4.9 提交前检查清单

```bash
# 1. 检查状态
git status

# 2. 查看更改
git diff

# 3. 暂存相关文件
git add <files>

# 4. 检查暂存内容
git diff --staged

# 5. 提交
git commit -m "feat: clear description"

# 6. 查看最近提交
git log -1 --stat

# 7. 推送
git push
```

---

## 5. 代码审查

### 5.1 Pull Request 模板

每个 PR 应该包含：

**标题**: `<type>: <description>`

**描述**:
```markdown
## 变更说明
简要描述这个 PR 的目的和内容。

## 变更类型
- [ ] 新功能
- [ ] Bug 修复
- [ ] 重构
- [ ] 文档更新
- [ ] 性能优化
- [ ] 其他: ______

## 测试
- [ ] 单元测试通过
- [ ] 手动测试完成
- [ ] 添加了新的测试用例

## 检查清单
- [ ] 代码遵循项目风格指南
- [ ] 提交消息符合规范
- [ ] 文档已更新（如需要）
- [ ] 无编译警告
- [ ] CI 检查通过

## 关联 Issue
Closes #GIT-xxx

## 截图（如适用）
<!-- 添加截图或演示 -->
```

### 5.2 审查清单

**代码质量**:
- [ ] 代码清晰易读
- [ ] 遵循项目编码规范
- [ ] 无重复代码
- [ ] 错误处理完善
- [ ] 性能考虑合理

**功能完整性**:
- [ ] 功能按预期工作
- [ ] 边界情况已处理
- [ ] 向后兼容性（如需要）

**测试**:
- [ ] 单元测试已添加
- [ ] 测试覆盖率足够
- [ ] 所有测试通过

**文档**:
- [ ] 代码注释充分
- [ ] API 文档已更新
- [ ] 用户文档已更新（如需要）

### 5.3 审查流程

1. **自我审查**: 提交 PR 前，作者自查
2. **自动检查**: CI 运行测试和静态分析
3. **同行审查**: 至少一名团队成员审查
4. **修改反馈**: 根据反馈修改代码
5. **批准合并**: 审查通过后合并

### 5.4 审查反馈指南

**建设性反馈**:
```markdown
# 好的反馈
"建议使用 `std::unique_ptr` 管理资源，避免内存泄漏"
"这个函数可以拆分成更小的函数提高可读性"
"考虑添加错误处理，如果 `plugin` 为 null 会怎样？"

# 避免的反馈
"这段代码不好" ❌
"重写" ❌
```

---

## 6. 发布流程

### 6.1 版本号规范

遵循 **Semantic Versioning 2.0.0**:

```
<major>.<minor>.<patch>

示例: 1.2.3
- major: 1 - 重大版本变更（不兼容的 API 修改）
- minor: 2 - 新功能（向后兼容）
- patch: 3 - Bug 修复（向后兼容）
```

### 6.2 发布决策树

```
是否包含破坏性变更？
├─ 是 → major 版本 (1.0.0 → 2.0.0)
└─ 否 → 是否包含新功能？
    ├─ 是 → minor 版本 (1.0.0 → 1.1.0)
    └─ 否 → patch 版本 (1.0.0 → 1.0.1)
```

### 6.3 发布前检查清单

**代码质量**:
- [ ] 所有测试通过
- [ ] 无严重 bug
- [ ] 代码审查完成
- [ ] 性能测试通过

**文档**:
- [ ] CHANGELOG.md 已更新
- [ ] README.md 已更新
- [ ] API 文档已更新
- [ ] 迁移指南（如需要）

**构建**:
- [ ] Windows 构建成功
- [ ] Linux 构建成功（如支持）
- [ ] macOS 构建成功（如支持）

**发布准备**:
- [ ] 版本号已更新
- [ ] 发布说明已撰写
- [ ] 发布标签已创建

### 6.4 CHANGELOG 格式

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-01-15

### Added
- Plugin system with dynamic loading
- Type-safe event bus for decoupled communication
- Configuration manager with JSON support
- Task manager for async operations
- ImHex-style content registry

### Changed
- Improved UI rendering performance
- Refactored config system for better scalability

### Fixed
- Memory leak in event subscription
- Crash on plugin unload

### Removed
- Old string-based event system

## [0.9.0] - 2024-01-01
...
```

---

## 7. 紧急修复

### 7.1 判断是否需要 Hotfix

**需要 Hotfix 的情况**:
- 生产环境崩溃
- 安全漏洞
- 数据丢失风险
- 严重影响用户体验

**不需要 Hotfix 的情况**:
- 非关键 bug
- 性能问题（非严重）
- 功能缺失
- UI 小问题

### 7.2 Hotfix 时间线

```
T+0:   发现问题
T+15m: 评估严重性，决定是否 hotfix
T+30m: 创建 hotfix 分支，开始修复
T+2h:  完成修复，测试通过
T+3h:  合并到 main，打标签，部署
T+4h:  合并回 develop，清理分支
```

### 7.3 Hotfix 最佳实践

**✅ DO**:
- 最小化更改范围
- 充分测试修复
- 添加回归测试
- 及时通知团队
- 记录问题和修复

**❌ DON'T**:
- 顺便添加新功能
- 重构相关代码
- 跳过测试
- 不通知直接部署
- 忘记合并回 develop

---

## 8. 常用命令

### 8.1 分支管理

```bash
# 创建新分支
git checkout -b feature/GIT-101-new-feature

# 列出所有分支
git branch -a

# 删除本地分支
git branch -d feature/GIT-101-new-feature

# 删除远程分支
git push origin --delete feature/GIT-101-new-feature

# 重命名分支
git branch -m old-name new-name

# 查看分支关系
git log --graph --oneline --all
```

### 8.2 同步与合并

```bash
# 拉取远程更改
git pull origin develop

# 拉取但不自动合并
git fetch origin develop

# 变基到最新的 develop
git rebase origin/develop

# 合并分支
git merge feature/GIT-101-new-feature

# 终止变基或合并
git rebase --abort
git merge --abort

# 继续变基或合并（解决冲突后）
git rebase --continue
git merge --continue
```

### 8.3 冲突解决

```bash
# 查看冲突文件
git status

# 编辑冲突文件
# 使用特殊标记:
# <<<<<<< HEAD
# 你的更改
# =======
# 他们的更改
# >>>>>>> feature/xxx

# 标记冲突已解决
git add <resolved-file>

# 查看解决后的更改
git diff --staged
```

### 8.4 历史查看

```bash
# 查看提交历史
git log

# 图形化显示
git log --graph --oneline --all

# 查看特定文件历史
git log -- path/to/file

# 查看谁修改了某行
git blame path/to/file

# 查看分支提交历史
git log develop..feature/xxx
```

### 8.5 撤销操作

```bash
# 撤销最后一次提交（保留更改）
git reset --soft HEAD~1

# 撤销最后一次提交（丢弃更改）
git reset --hard HEAD~1

# 撤销特定提交（创建新提交）
git revert <commit-hash>

# 撤销已推送的提交
git revert <commit-hash>
git push
```

### 8.6 临时保存工作

```bash
# 保存当前工作
git stash

# 保存并添加消息
git stash save "work in progress"

# 查看 stash 列表
git stash list

# 恢复 stash
git stash pop

# 恢复特定 stash
git stash apply stash@{0}

# 删除 stash
git stash drop stash@{0}
```

### 8.7 标签管理

```bash
# 列出所有标签
git tag

# 创建轻量标签
git tag v1.0.0

# 创建附注标签
git tag -a v1.0.0 -m "Release version 1.0.0"

# 推送标签到远程
git push origin v1.0.0

# 推送所有标签
git push origin --tags

# 删除标签
git tag -d v1.0.0
git push origin --delete v1.0.0

# 查看标签信息
git show v1.0.0
```

---

## 9. 团队协作最佳实践

### 9.1 日常开发流程

```bash
# 每天开始工作前
git checkout develop
git pull origin develop

# 创建功能分支
git checkout -b feature/GIT-xxx-feature-name

# 开发过程中
git add .
git commit -m "feat: description"
git push

# 定期同步 develop
git fetch origin develop
git rebase origin/develop
```

### 9.2 提交前自检

```bash
# 运行测试
ctest --test-dir build

# 代码格式化
clang-format -i src/*.cpp

# 静态分析
clang-tidy src/*.cpp

# 检查构建
cmake --build build
```

### 9.3 代码审查礼仪

**作为作者**:
- 保持 PR 小而专注
- 及时响应反馈
- 解释复杂的设计决策
- 感谢审查者时间

**作为审查者**:
- 及时审查 PR
- 给出建设性反馈
- 肯定好的做法
- 提出改进建议

### 9.4 处理冲突

```bash
# 1. 拉取最新代码
git fetch origin

# 2. 变基到目标分支
git rebase origin/develop

# 3. 解决冲突
# 编辑文件，解决冲突标记

# 4. 标记冲突已解决
git add <resolved-files>

# 5. 继续变基
git rebase --continue

# 6. 如果变基复杂，可以放弃重新开始
git rebase --abort
git merge origin/develop
# 手动解决冲突
git merge --continue

# 7. 推送（可能需要 force）
git push --force-with-lease
```

---

## 10. CI/CD 集成

### 10.1 GitHub Actions 工作流

`.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [ develop, main ]
  pull_request:
    branches: [ develop, main ]

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3

    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }}

    - name: Test
      run: ctest --test-dir build --config ${{ matrix.build_type }}
```

### 10.2 自动化检查

每次 Push 或 PR 时，CI 自动检查：
- ✅ 代码编译成功
- ✅ 所有测试通过
- ✅ 代码风格检查
- ✅ 静态分析通过
- ✅ 无内存泄漏

### 10.3 保护规则

**分支保护设置** (在 GitHub 仓库设置中):

**main 分支**:
- ✅ Require pull request before merging
  - Require approvals: 1
- ✅ Require status checks to pass before merging
  - CI/build-and-test
- ✅ Require branches to be up to date before merging
- ❌ Do not allow bypassing the above settings

**develop 分支**:
- ✅ Require pull request before merging
  - Require approvals: 1
- ✅ Require status checks to pass before merging

---

## 11. 故障排查

### 11.1 常见问题

**问题 1: 推送被拒绝**
```bash
error: failed to push some refs to 'https://github.com/...'
```

**解决方案**:
```bash
# 先拉取远程更改
git pull --rebase origin develop

# 如果有冲突，解决后
git add .
git rebase --continue

# 再推送
git push
```

**问题 2: 变基冲突**
```bash
ERROR: Could not apply <commit-hash>... <commit-message>
```

**解决方案**:
```bash
# 1. 查看冲突文件
git status

# 2. 手动编辑冲突文件

# 3. 标记冲突已解决
git add <file>

# 4. 继续变基
git rebase --continue

# 5. 如果太复杂，放弃变基
git rebase --abort
# 使用 merge 代替
```

**问题 3: 意外提交到错误分支**
```bash
# 当前在 main，但应该提交到 develop
```

**解决方案**:
```bash
# 1. 创建正确的分支
git checkout -b feature/xxx

# 2. 切换回 main
git checkout main

# 3. 重置到远程
git reset --hard origin/main

# 4. 切换到功能分支
git checkout feature/xxx

# 5. 正常开发
```

---

## 12. 参考资源

### 12.1 官方文档

- [Git 官方文档](https://git-scm.com/doc)
- [GitHub Flow](https://docs.github.com/en/get-started/quickstart/github-flow)
- [Git Flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Semantic Versioning](https://semver.org/)

### 12.2 团队文档

- [DearTs 架构文档](../docs/architecture.md)
- [插件系统指南](../docs/plugin_system_guide.md)
- [API 参考手册](./references/plugin_system_api.md)

### 12.3 有用工具

- **Git 客户端**:
  - [GitKraken](https://www.gitkraken.com/) - 图形化 Git 客户端
  - [Sourcetree](https://www.sourcetreeapp.com/) - 免费的 Git GUI
  - [VS Code Git](https://code.visualstudio.com/docs/sourcecontrol/overview) - 内置 Git 支持

- **提交规范工具**:
  - [Commitizen](https://commitizen.github.io/cz-cli/) - 交互式提交工具
  - [Husky](https://github.com/typicode/husky) - Git hooks 管理
  - [Commitlint](https://commitlint.js.org/) - 提交消息检查

---

## 13. 快速参考

### 13.1 工作流程速查表

| 任务 | 命令 |
|------|------|
| 开始新功能 | `git checkout develop && git pull && git checkout -b feature/xxx` |
| 提交更改 | `git add . && git commit -m "feat: description"` |
| 同步最新代码 | `git fetch origin develop && git rebase origin/develop` |
| 创建 PR | 推送分支后在 GitHub 创建 |
| 完成功能 | 合并 PR 后 `git branch -d feature/xxx` |
| 紧急修复 | `git checkout main && git pull && git checkout -b hotfix/xxx` |
| 发布新版本 | `git checkout develop && git checkout -b release/x.y.z` |

### 13.2 提交类型速查

| Type | 使用场景 |
|------|----------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档 |
| `style` | 代码格式 |
| `refactor` | 重构 |
| `perf` | 性能优化 |
| `test` | 测试 |
| `chore` | 构建/工具 |
| `ci` | CI 配置 |

---

## 附录 A: Git 配置示例

### A.1 全局配置

```bash
# 设置用户信息
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 设置默认分支名
git config --global init.defaultBranch develop

# 设置拉取策略
git config --global pull.rebase true

# 设置推送策略
git config --global push.default simple

# 设置别名
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.st status
git config --global alias.lg "log --graph --oneline --all"
```

### A.2 项目配置

```bash
# 在项目根目录
git config core.autocrlf false  # Windows
git config core.autocrlf input  # Linux/Mac

# 设置合并工具
git config merge.tool vscode
git config mergetool.vscode.cmd 'code --wait $MERGED'
```

---

## 附录 B: PR 模板文件

`.github/pull_request_template.md`:

```markdown
---
## 变更说明
<!-- 简要描述这个 PR 的目的 -->

## 变更类型
- [ ] `feat`: 新功能
- [ ] `fix`: Bug 修复
- [ ] `docs`: 文档更新
- [ ] `style`: 代码格式（不影响功能）
- [ ] `refactor`: 重构
- [ ] `perf`: 性能优化
- [ ] `test`: 测试
- [ ] `chore`: 构建/工具

## 测试
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] 手动测试完成
- [ ] 添加了新测试

## 检查清单
- [ ] 代码遵循项目规范
- [ ] 提交消息符合规范
- [ ] 文档已更新（如需要）
- [ ] 无编译警告
- [ ] CI 检查通过

## 关联 Issue
Closes #(issue number)

## 截图/演示
<!-- 如果适用，添加截图或 GIF -->

## 其他信息
<!-- 任何审查者需要知道的信息 -->
```

---

## 附录 C: Issue 模板

`.github/ISSUE_TEMPLATE/bug_report.md`:

```markdown
---
name: Bug report
about: 创建 bug 报告帮助我们改进
title: '[BUG] '
labels: bug
assignees: ''
---

## Bug 描述
<!-- 清晰简洁地描述 bug -->

## 复现步骤
1. 前往 '...'
2. 点击 '....'
3. 滚动到 '....'
4. 看到错误

## 期望行为
<!-- 描述你期望发生的行为 -->

## 截图
<!-- 如果适用，添加截图 -->

## 环境信息
- OS: [e.g. Windows 11, Ubuntu 22.04]
- DearTs Version: [e.g. 1.0.0]
- Compiler: [e.g. MSVC 2022, GCC 11]

## 额外信息
<!-- 添加任何其他相关信息 -->
```

---

**文档版本**: 1.0.0
**最后更新**: 2024-12-30
**维护者**: DearTs Development Team

---

## 快速开始

如果你是新手，从这里开始：

1. **克隆仓库**
   ```bash
   git clone https://github.com/ygsheep/DearTs.git
   cd DearTs
   ```

2. **设置开发环境**
   ```bash
   git checkout develop
   git remote add upstream https://github.com/ygsheep/DearTs.git
   ```

3. **开始第一个功能**
   ```bash
   git checkout -b feature/my-first-feature
   # 进行开发...
   git add .
   git commit -m "feat: add my first feature"
   git push -u origin feature/my-first-feature
   ```

4. **创建 Pull Request**
   - 在 GitHub 上打开 PR
   - 等待代码审查
   - 根据反馈修改

5. **合并并清理**
   ```bash
   git checkout develop
   git pull origin develop
   git branch -d feature/my-first-feature
   ```

欢迎加入 DearTs Framework 开发！🚀
