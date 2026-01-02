# 双代理协作框架 (Dual-Agent Framework)

> **Claude Code 技能包** - 基于 TDD 和外部化状态的长期任务管理框架

## 技能结构

```
agent-framework/
├── SKILL.md              # 主技能文件（快速入口）
├── README.md             # 本文件（结构说明）
└── (待扩展: references/, scripts/, examples/)
```

---

## 快速开始

### 查看主技能文件

**[SKILL.md](SKILL.md)** - 技能主入口

包含:
- 框架概述
- 双代理角色说明
- 核心文件说明
- 工作流程
- 关键约束
- 完整示例

**适用场景:**
- 长期编程任务管理
- 测试驱动开发（TDD）
- 功能清单追踪
- Git 原子性提交
- AI 协作编程

---

## 核心概念

### 问题背景

AI 助手在长期任务中面临两个核心问题：

1. **上下文失忆** - 多次对话后忘记之前的状态
2. **虚假完成** - 声称完成但实际未通过测试

### 解决方案

**外部化状态** - 将状态持久化到文件系统：
- `cloud_demand.md` - 需求文档
- `featurelist.json` - 功能清单
- `progress.md` - 进度报告

**测试驱动** - 强制测试通过才算完成：
- `test_status == "passed"` 才能标记完成
- 每个功能独立 Git 提交
- 提交必须包含测试结果

---

## 使用场景

### 适用项目

- 大型功能开发（30+ 项功能）
- 需要多会话协作的任务
- 测试驱动的开发流程
- 需要详细进度追踪的项目

### 技术栈

- 任何编程语言（C++, Python, JavaScript 等）
- 任何构建系统（CMake, npm, cargo 等）
- 任何测试框架（Google Test, pytest, Jest 等）
- Git 版本控制

---

## 代理职责

### 代理 A：初始化/管理代理

负责项目规划：
1. 创建需求文档 (`cloud_demand.md`)
2. 生成功能清单 (`featurelist.json`)
3. 定义 Git 提交规范
4. 监控编码代理进度

### 代理 B：编码/测试代理

负责功能实现：
1. 遍历功能清单实施功能
2. 编写代码 + 测试
3. 更新测试状态
4. 提交 Git 并更新进度

---

## 核心文件

### cloud_demand.md

需求的单一真实来源，包含：
- 项目概述
- 系统约束
- 功能需求（分 Phase）
- 验收标准
- 禁止事项

### featurelist.json

结构化功能清单，包含：
- 功能 ID、标题、描述
- 优先级、依赖关系
- 测试用例
- 实现状态、测试状态
- Git 提交哈希
- 预估/实际时间

### progress.md

自动生成的进度报告，包含：
- 整体进度统计
- Phase 进度详情
- Git 提交历史

---

## 工作流程

### 编码代理循环

```
1. 读取状态 (featurelist.json)
   ↓
2. 选择下一个任务 (检查依赖)
   ↓
3. 实施功能 (代码 + 测试)
   ↓
4. 运行测试
   ↓
5. 更新状态 (test_status)
   ↓
6. Git 提交
   ↓
7. 更新进度 (progress.md)
   ↓
8. 返回步骤 1
```

---

## 关键约束

### 测试驱动（TDD）

!> **禁止虚假完成**
- 测试通过前不能标记完成
- 测试失败必须记录到 featurelist.json
- 所有测试用例必须通过

### Git 原子性

!> **禁止批量提交**
- 每个功能独立提交
- 提交必须包含功能 ID
- 提交必须包含测试结果

### 状态一致性

!> **三文件同步**
- featurelist.json - 单一数据源
- progress.md - 自动生成报告
- Git log - 不可变历史

---

## 自动化工具

### 进度生成脚本

```bash
python scripts/generate_progress.py
```

自动从 featurelist.json 生成 progress.md

### Git 提交模板

```bash
git config commit.template .gitcommit
```

提交时自动加载规范格式

### Pre-commit Hook（可选）

检查：
- featurelist.json 语法
- 测试状态一致性

---

## 扩展计划

### 计划添加

- `references/` - 详细 API 文档
- `scripts/` - 自动化工具脚本
- `examples/` - 完整示例项目
- `templates/` - 项目模板

---

**技能版本**: 1.0.0
**最后更新**: 2026-01-01
**维护者**: DearTs Team
