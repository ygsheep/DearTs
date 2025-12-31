# DearTs Framework - Development Skill

> **Claude Code 技能包** - DearTs Framework 开发辅助技能

## 📁 技能结构

```
dearts-dev/
├── SKILL.md              # 主技能文件（快速入口）
├── README.md             # 本文件（结构说明）
├── references/           # API 参考文档（详细手册）
├── assets/               # 代码模板和架构图
├── scripts/              # 开发工具脚本
├── examples/             # 示例代码和教程
└── archive/              # 历史文档存档
```

---

## 🚀 快速开始

### 1. 查看主技能文件

**[SKILL.md](SKILL.md)** - 技能主入口

包含:
- 框架概述
- 快速开始示例
- 核心系统架构
- 参考文档索引
- 最佳实践

**适用场景:**
- 初次使用 DearTs Framework
- 需要快速查找信息
- 了解系统架构

---

### 2. 查阅 API 文档

**[references/](references/)** - 详细 API 手册

优先阅读（核心 API 手册）:
- **[config_manager_api.md](references/config_manager_api.md)** - ConfigManager 完全手册
- **[logger_api.md](references/logger_api.md)** - Logger 完全手册
- **[task_manager_api.md](references/task_manager_api.md)** - TaskManager 完全手册
- **[plugin_system_api.md](references/plugin_system_api.md)** - Plugin System 完全手册

**详细索引**: [references/README.md](references/README.md)

---

### 3. 使用代码模板

**[assets/](assets/)** - 代码模板和架构图

包含:
- `app_template.cpp` - 应用程序模板
- `view_template.cpp` - 视图模板
- `plugin_template.cpp` - 插件模板
- `cmake_template.txt` - CMake 模板
- 14 个架构图 (PNG)

**详细说明**: [assets/README.md](assets/README.md)

---

### 4. 运行示例

**[examples/](examples/)** - 示例代码和教程

包含:
- SDL3 + ImGui 混合渲染指南
- 交互式 SDL 渲染完整示例
- 源代码文件

**详细说明**: [examples/README.md](examples/README.md)

---

### 5. 使用构建脚本

**[scripts/](scripts/)** - 开发工具脚本

包含:
- `build.py` - Python 构建脚本（推荐）
- `build_debug.bat` - Windows Debug 构建
- `build_temp.bat` - Windows 临时构建

**详细说明**: [scripts/README.md](scripts/README.md)

---

## 📚 文档使用指南

### 按任务查找

| 任务 | 文档 |
|------|------|
| **创建应用** | [SKILL.md - 快速开始](SKILL.md) |
| **创建插件** | [references/plugin_system_api.md](references/plugin_system_api.md) |
| **管理配置** | [references/config_manager_api.md](references/config_manager_api.md) |
| **异步任务** | [references/task_manager_api.md](references/task_manager_api.md) |
| **添加日志** | [references/logger_api.md](references/logger_api.md) |
| **事件系统** | [references/event_system.md](references/event_system.md) |
| **注册命令** | [references/content_registry.md](references/content_registry.md) |
| **创建视图** | [references/ui_system.md](references/ui_system.md) |

### 按角色查找

**新手开发者:**
1. 阅读 [SKILL.md](SKILL.md) 了解框架
2. 复制 [assets/app_template.cpp](assets/app_template.cpp) 创建应用
3. 参考 [references/plugin_system_api.md](references/plugin_system_api.md) 创建插件

**中级开发者:**
1. 查阅 [references/](references/) 中的 API 手册
2. 研究 [examples/](examples/) 中的示例代码
3. 使用 [scripts/build.py](scripts/build.py) 构建项目

**高级开发者:**
1. 深入阅读所有 API 文档
2. 参考架构图 ([assets/*.png](assets/))
3. 贡献代码和文档

---

## 🎯 技能设计原则

### 渐进式披露 (Progressive Disclosure)

```
1. 元数据（YAML frontmatter）
   ↓ 快速判断是否加载
2. SKILL.md 主体（<500 行）
   ↓ 快速浏览了解框架
3. references/ 详细文档
   ↓ 按需查阅 API 手册
4. assets/ templates
   ↓ 获取代码模板
5. examples/ 代码
   ↓ 学习实际实现
```

### 文档分层

| 层级 | 文件 | 行数 | 用途 |
|------|------|------|------|
| **1. 元数据** | SKILL.md YAML | 4 行 | 触发条件 |
| **2. 快速入口** | SKILL.md | 338 行 | 快速查找 |
| **3. 详细手册** | references/*.md | ~7158 行 | 深入学习 |
| **4. 代码模板** | assets/*.cpp | ~10 KB | 快速开始 |
| **5. 示例代码** | examples/ | ~30 KB | 实战参考 |

---

## 📊 技能统计

| 类别 | 数量 | 总行数 | 总大小 |
|------|------|--------|--------|
| 核心 API 手册 | 4 | 3358 | 80 KB |
| 其他参考文档 | 11 | ~3800 | ~75 KB |
| 代码模板 | 4 | ~400 | ~10 KB |
| 示例代码 | 4 | ~800 | ~30 KB |
| 架构图 | 14 | - | 1.3 MB |
| **总计** | **37** | **~8358** | **~1.5 MB** |

---

## 🔗 外部链接

- **项目文档**: `../../docs/`
- **插件示例**: `../../plugins/builtin/`
- **SDL3 文档**: https://wiki.libsdl.org/SDL3/
- **ImGui GitHub**: https://github.com/ocornut/imgui
- **ImHex GitHub**: https://github.com/WerWolv/ImHex
- **CMake 文档**: https://cmake.org/documentation/
- **C++ 参考**: https://en.cppreference.com/w/cpp

---

## 📝 版本历史

### v3.0.0 (2025-12-30)
- ✅ 重构技能结构，遵循 Agent Skills 教程
- ✅ 精简 SKILL.md 到 338 行（<500 行目标）
- ✅ 添加渐进式文档结构
- ✅ 创建各目录 README 索引
- ✅ 归档历史文档到 archive/

### v2.1.0 (2025-12-28)
- ✅ 添加 4 个核心 API 手册（3358 行）
- ✅ 更新 SKILL.md 添加插件系统章节

### v2.0.0 (2025-12-27)
- ✅ 添加 ConfigManager, Logger, TaskManager 文档
- ✅ 更新技能内容

### v1.0.0 (2025-12-20)
- ✅ 初始版本

---

**技能版本**: 3.0.0
**最后更新**: 2025-12-30
**框架版本**: DearTs Framework 1.0.0
**维护者**: DearTs Team
