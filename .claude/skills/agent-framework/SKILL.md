---
name: agent-framework
description: 双代理协作框架 - 基于外部化状态和测试驱动开发（TDD）的长期任务管理框架。解决 AI 助手的上下文失忆和虚假完成问题。适用于需要多会话协作的复杂编程项目、功能追踪、测试驱动的开发场景。
---

# 双代理协作框架 (Dual-Agent Framework)

## 概述

这是一个基于**外部化状态**和**测试驱动开发（TDD）**的双代理协作框架，用于管理长期编程任务，解决 AI 助手的两个核心问题：

1. **上下文失忆** - 通过文件系统持久化状态
2. **虚假完成** - 强制测试通过才算完成

---

## 文件结构

```
Project/
├── cloud_demand.md           # 原始需求文档（初始化代理创建）
├── featurelist.json          # 功能清单（状态追踪）
├── progress.md               # 进度报告（自动生成）
├── .gitcommit                # Git 提交模板
└── scripts/
    └── generate_progress.py  # 进度生成脚本
```

---

## 双代理角色

### 代理 A：初始化/管理代理

**职责**:
1. 创建需求文档 (`cloud_demand.md`)
2. 生成功能清单 (`featurelist.json`)
3. 初始化进度文件 (`progress.md`)
4. 定义 Git 提交规范
5. 监控编码代理进度

### 代理 B：编码/测试代理

**职责**:
1. 读取 `cloud_demand.md` 理解需求
2. 遍历 `featurelist.json` 实施功能
3. 实现代码 + 编写测试
4. 更新 `featurelist.json` 测试状态
5. 提交 Git 并更新 `progress.md`

**工作流程**:
```python
for feature in featurelist.json:
    if feature.dependencies_completed():
        implement(feature)
        test_results = run_tests(feature.test_cases)
        feature.test_status = "passed" if test_results.all_passed else "failed"
        git_commit(feature)
        update_progress(feature)
```

---

## 核心文件说明

### 1. `cloud_demand.md`

**用途**: 需求的**单一真实来源**（Single Source of Truth）

**内容**:
- 项目概述
- 系统约束（技术栈、架构、许可证）
- 功能需求（多个 Phase）
- 验收标准
- 禁止事项

**示例**:
```markdown
## 系统约束
!> **强制要求**：必须使用 vcpkg 包管理器集成 FFmpeg
!> **平台支持**：必须同时支持 Windows 11 和 Linux

## 功能需求
### Phase 1: 基础设施
- [ ] 安装和配置 vcpkg
- [ ] 安装 FFmpeg 包
```

### 2. `featurelist.json`

**用途**: 结构化**功能清单 + 测试状态**

**结构**:
```json
{
  "project": "Project-Name",
  "version": "1.0.0",
  "last_updated": "2025-01-01T12:00:00Z",
  "features": [
    {
      "id": "phase2-02",
      "phase": 2,
      "title": "实现功能名称",
      "priority": "P1",
      "desc": "功能描述",
      "test_cases": [
        "测试用例1",
        "测试用例2"
      ],
      "implementation_status": "pending",
      "test_status": "pending",
      "git_commit": null,
      "estimated_hours": 1.0,
      "actual_hours": null,
      "dependencies": ["phase2-01"],
      "notes": ""
    }
  ],
  "statistics": {
    "total_features": 30,
    "completed_features": 0,
    "passed_tests": 0,
    "total_estimated_hours": 30.5
  }
}
```

**关键字段**:
- `implementation_status`: 实现状态
- `test_status`: 测试状态（**必须通过才算完成**）
- `dependencies`: 依赖的功能 ID
- `git_commit`: Git 提交哈希

### 3. `progress.md`

**用途**: 人类可读的**进度报告**（自动生成）

**生成方式**:
```bash
python scripts/generate_progress.py
```

---

## 工作流程

### 编码代理工作流程

#### 步骤 1: 读取状态
```bash
# 查看需求
cat cloud_demand.md

# 查看任务列表
cat featurelist.json | jq '.features[] | select(.implementation_status == "pending")'

# 查看进度
cat progress.md

# 查看最近提交
git log --oneline -10
```

#### 步骤 2: 选择下一个任务
```python
# 伪代码
next_task = get_next_pending_task(featurelist.json)

# 检查依赖
if not all(dep.test_status == "passed" for dep in next_task.dependencies):
    skip_task()  # 依赖未满足，跳过
else:
    execute_task(next_task)
```

#### 步骤 3: 实施功能
```bash
# 1. 阅读需求
grep -A 20 "phase2-02" cloud_demand.md

# 2. 实现代码
# ... 编写代码 ...

# 3. 编写测试
# ... 编写测试 ...

# 4. 运行测试
cmake --build build
ctest --test-dir build --verbose
```

#### 步骤 4: 更新 featurelist.json
```json
{
  "id": "phase2-02",
  "implementation_status": "completed",
  "test_status": "passed",
  "git_commit": "abc123",
  "actual_hours": 1.2,
  "completed_at": "2025-01-01T14:30:00Z"
}
```

#### 步骤 5: Git 提交
```bash
# 添加文件
git add <source_files>
git add <test_files>
git add featurelist.json

# 提交（使用模板）
git commit -F .gitcommit
```

#### 步骤 6: 更新进度
```bash
# 生成 progress.md
python scripts/generate_progress.py

# 提交进度更新
git add progress.md
git commit -m "chore: update progress"
```

---

## 关键约束

### 测试驱动开发（TDD）

!> **禁止虚假完成**:
- `test_status == "passed"` 之前，`implementation_status` 不能为 `"completed"`
- 所有测试用例必须通过
- 测试失败时必须记录到 `featurelist.json`

### Git 原子性

!> **禁止批量提交**:
- 每个功能项独立提交
- 提交消息必须包含功能 ID
- 提交消息必须包含测试结果

### 状态一致性

!> **三文件同步**:
- `featurelist.json` - 单一数据源
- `progress.md` - 人类可读报告（自动生成）
- Git log - 不可变历史

---

## 快速开始

### 查看当前状态
```bash
# 查看功能清单
cat featurelist.json | jq '.statistics'

# 查看进度
cat progress.md

# 查看最近提交
git log --oneline -10
```

### 开始编码
```bash
# 1. 选择下一个任务
cat featurelist.json | jq '.features[] | select(.implementation_status == "pending" and .dependencies == [])'

# 2. 实施功能
# ... 编写代码 ...

# 3. 运行测试
cmake --build build
ctest --test-dir build --verbose

# 4. 更新状态
# 编辑 featurelist.json

# 5. 提交
git add <files>
git commit -F .gitcommit

# 6. 更新进度
python scripts/generate_progress.py
git add progress.md
git commit -m "chore: update progress"
```

---

## 完整工作流示例

### 场景：实现某功能

```bash
# === 步骤 1: 选择任务 ===
cat featurelist.json | jq '.features[] | select(.id == "phase2-02")'

# === 步骤 2: 阅读需求 ===
grep -A 30 "phase2-02" cloud_demand.md

# === 步骤 3: 实施功能 ===
# 创建文件并实现代码

# === 步骤 4: 编写测试 ===
# 创建测试文件

# === 步骤 5: 编译和测试 ===
cmake --build build
ctest --test-dir build --verbose

# === 步骤 6: 更新 featurelist.json ===
# 更新状态为 completed 和 test_status 为 passed

# === 步骤 7: Git 提交 ===
git add <files>
git add featurelist.json
git commit -F .gitcommit

# === 步骤 8: 更新进度 ===
python scripts/generate_progress.py
git add progress.md
git commit -m "chore: update progress"
```

---

## 技能使用场景

此技能在以下场景自动激活：

- 管理长期编程任务（需要多会话协作）
- 实现功能清单追踪系统
- 使用测试驱动开发（TDD）方法
- 需要 Git 原子性提交管理
- 需要外部化状态避免上下文失忆
- 创建进度报告系统
- 管理依赖关系复杂的功能开发

---

**框架版本**: 1.0.0
**创建时间**: 2025-01-01
**适用场景**: 长期任务管理、TDD 开发、AI 协作编程
