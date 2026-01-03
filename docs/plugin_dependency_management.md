# DearTs Framework - 插件依赖管理系统指南

## 目录

1. [系统概述](#系统概述)
2. [依赖类型](#依赖类型)
3. [版本范围语法](#版本范围语法)
4. [如何在插件中声明依赖](#如何在插件中声明依赖)
5. [完整示例](#完整示例)
6. [API 参考](#api-参考)
7. [最佳实践](#最佳实践)
8. [故障排除](#故障排除)

---

## 系统概述

DearTs Framework 插件依赖管理系统提供了强大的插件间依赖解析功能，支持：

- ✅ **语义化版本控制** (SemVer 2.0.0)
- ✅ **灵活的版本范围** (npm 风格)
- ✅ **多种依赖类型** (必需、可选、软依赖)
- ✅ **自动依赖解析** (拓扑排序)
- ✅ **循环依赖检测**
- ✅ **版本冲突验证**
- ✅ **完全向后兼容**

### 核心特性

| 特性 | 说明 |
|------|------|
| **自动发现** | 从 `plugins/` 目录自动加载插件 |
| **依赖解析** | 自动计算插件加载顺序 |
| **版本验证** | 确保依赖版本满足要求 |
| **错误报告** | 详细的错误信息和依赖链 |
| **双模式** | 宽松模式（默认）和严格模式 |

---

## 依赖类型

系统支持三种依赖类型，满足不同的集成需求：

### 1. Required（必需依赖）

插件**必须**拥有此依赖才能运行。

- **缺失行为**: 插件加载失败，显示错误
- **版本不匹配**: 插件加载失败，显示错误
- **适用场景**: 核心功能依赖

```cpp
PluginDependency::required("CorePlugin", ">=2.0.0")
```

### 2. Optional（可选依赖）

插件可以**增强功能**，但不是必需的。

- **缺失行为**: 插件加载，记录警告
- **版本不匹配**: 插件加载，记录警告
- **适用场景**: 可选功能增强

```cpp
PluginDependency::optional("UIEnhancementPlugin", "^1.5.0")
```

### 3. Soft（软依赖）

插件可以**利用**其他插件的功能，但不会主动检查。

- **缺失行为**: 插件加载，**静默**（无警告）
- **版本不匹配**: 插件加载，**静默**
- **适用场景**: 被动集成，不影响核心功能

```cpp
PluginDependency::soft("LoggerPlugin", ">=1.0.0")
```

### 依赖类型对比表

| 类型 | 缺失时 | 版本冲突时 | 日志级别 | 使用场景 |
|------|---------|-----------|----------|----------|
| **Required** | ❌ 加载失败 | ❌ 加载失败 | ERROR | 核心依赖 |
| **Optional** | ⚠️ 警告 | ⚠️ 警告 | WARN | 功能增强 |
| **Soft** | ✅ 静默 | ✅ 静默 | DEBUG | 被动集成 |

---

## 版本范围语法

系统支持完整的 npm 风格版本范围（基于 SemVer 2.0.0）。

### 基础语法

| 语法 | 说明 | 示例 | 匹配版本 |
|------|------|------|----------|
| `1.2.3` | 精确版本 | `1.2.3` | `1.2.3` ✅<br>`1.2.4` ❌ |
| `>=1.2.3` | 大于或等于 | `>=1.2.3` | `1.2.3` ✅<br>`1.3.0` ✅<br>`2.0.0` ✅ |
| `>1.2.3` | 大于 | `>1.2.3` | `1.2.4` ✅<br>`1.2.3` ❌ |
| `<=1.2.3` | 小于或等于 | `<=1.2.3` | `1.2.3` ✅<br>`1.2.2` ✅<br>`1.3.0` ❌ |
| `<1.2.3` | 小于 | `<1.2.3` | `1.2.2` ✅<br>`1.2.3` ❌ |

### 高级语法

| 语法 | 说明 | 范围 | 示例 |
|------|------|------|------|
| `^1.2.3` | 兼容版本 | `>=1.2.3 <2.0.0` | 不影响 public API 的更新 |
| `~1.2.3` | 补丁更新 | `>=1.2.3 <1.3.0` | 只允许补丁级别更新 |
| `1.2.3 - 2.3.4` | 连字符范围 | `>=1.2.3 <=2.3.4` | 包含范围的版本 |
| `1.2.*` | 通配符 | `>=1.2.0 <1.3.0` | 任何 1.2.x 版本 |
| `1.x` | 主版本通配符 | `>=1.0.0 <2.0.0` | 任何 1.x 版本 |
| `*` | 任意版本 | 所有版本 | 不限制版本 |

### 组合条件

可以使用空格组合多个条件（AND 逻辑）：

```cpp
">=1.2.3 <2.0.0"  // 1.2.3 到 2.0.0 之前
">=1.2.3 !=1.2.5" // 1.2.3+ 除了 1.2.5
```

### 实际示例

```cpp
// 精确版本
PluginDependency::required("Plugin", "1.2.3")

// 兼容版本（推荐）
PluginDependency::required("Plugin", "^1.2.3")
// 匹配: 1.2.3, 1.2.4, 1.5.0, 1.9.9
// 不匹配: 2.0.0, 0.9.0

// 补丁更新
PluginDependency::optional("Plugin", "~1.2.3")
// 匹配: 1.2.3, 1.2.4, 1.2.99
// 不匹配: 1.3.0, 2.0.0

// 范围
PluginDependency::soft("Plugin", "1.2.3 - 2.0.0")
// 匹配: 1.2.3 到 2.0.0 之间（含）

// 通配符
PluginDependency::required("Plugin", "1.2.*")
// 匹配: 任何 1.2.x 版本
```

---

## 如何在插件中声明依赖

### 步骤 1: 继承 IPlugin

确保你的插件类继承自 `IPlugin`：

```cpp
#include "core/plugin/plugin.h"

using namespace DearTs::Core::Plugin;

class MyPlugin : public IPlugin {
public:
    // ... 实现
};
```

### 步骤 2: 实现 get_dependencies() 方法

添加 `get_dependencies()` 虚方法：

```cpp
#include "core/plugin/plugin_dependency.h"

class MyPlugin : public IPlugin {
public:
    [[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
        // 声明依赖
        return {
            // ... 依赖列表
        };
    }
};
```

### 步骤 3: 使用工厂方法创建依赖

使用 `PluginDependency` 的静态工厂方法：

```cpp
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    std::vector<PluginDependency> deps;

    // 必需依赖
    auto dep1 = PluginDependency::required("CorePlugin", ">=2.0.0");
    if (dep1.isOk()) {
        deps.push_back(dep1.unwrap());
    }

    // 可选依赖
    auto dep2 = PluginDependency::optional("UIPlugin", "^1.5.0");
    if (dep2.isOk()) {
        deps.push_back(dep2.unwrap());
    }

    // 软依赖
    auto dep3 = PluginDependency::soft("LoggerPlugin", ">=1.0.0");
    if (dep3.isOk()) {
        deps.push_back(dep3.unwrap());
    }

    return deps;
}
```

### 步骤 4: 返回依赖列表

返回包含所有依赖的向量：

```cpp
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {
        PluginDependency::required("CorePlugin", ">=2.0.0").unwrap(),
        PluginDependency::optional("UIPlugin", "^1.5.0").unwrap(),
        PluginDependency::soft("LoggerPlugin", ">=1.0.0").unwrap()
    };
}
```

---

## 完整示例

### 示例 1: 简单插件（无依赖）

```cpp
#pragma once

#include "core/plugin/plugin.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"

using namespace DearTs;
using namespace DearTs::Core;

namespace SimplePlugin {

class SimplePlugin : public Plugin::IPlugin {
public:
    [[nodiscard]] Plugin::PluginInfo get_info() const override {
        return Plugin::PluginInfo{
            .name = "SimplePlugin",
            .author = "Your Name",
            .description = "A simple plugin without dependencies",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 无需声明依赖 - 默认返回空列表
    // [[nodiscard]] std::vector<Plugin::PluginDependency> get_dependencies() const override {
    //     return {};
    // }

    Result<void, std::string> on_load() override {
        LOG_INFO("SimplePlugin loaded successfully");
        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        LOG_INFO("SimplePlugin unloaded");
    }
};

} // namespace SimplePlugin
```

### 示例 2: 带必需依赖的插件

```cpp
#pragma once

#include "core/plugin/plugin.h"
#include "core/plugin/plugin_dependency.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"

using namespace DearTs;
using namespace DearTs::Core;

namespace AdvancedPlugin {

class AdvancedPlugin : public Plugin::IPlugin {
public:
    [[nodiscard]] Plugin::PluginInfo get_info() const override {
        return Plugin::PluginInfo{
            .name = "AdvancedPlugin",
            .author = "Your Name",
            .description = "Plugin with required dependencies",
            .version = "2.0.0",
            .api_version = "1.0.0"
        };
    }

    [[nodiscard]] std::vector<Plugin::PluginDependency> get_dependencies() const override {
        return {
            PluginDependency::required("BuiltinPlugin", ">=1.0.0").unwrap()
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("AdvancedPlugin loaded - BuiltinPlugin is available");

        ContentRegistry::Commands::add(
            "advanced.feature",
            "Advanced Feature",
            []() {
                LOG_INFO("Advanced feature executed!");
            }
        );

        return Result<void, std::string>::ok();
    }
};

} // namespace AdvancedPlugin
```

### 示例 3: 多种依赖类型

```cpp
[[nodiscard]] std::vector<Plugin::PluginDependency> get_dependencies() const override {
    std::vector<Plugin::PluginDependency> deps;

    // 1. 必需依赖：基础插件
    auto base_dep = PluginDependency::required("BasePlugin", ">=3.0.0");
    if (base_dep.isOk()) {
        deps.push_back(base_dep.unwrap());
    }

    // 2. 可选依赖：UI 增强
    auto ui_dep = PluginDependency::optional("UIEnhancementPlugin", "^2.1.0");
    if (ui_dep.isOk()) {
        deps.push_back(ui_dep.unwrap());
    }

    // 3. 可选依赖：数据导出
    auto export_dep = PluginDependency::optional("DataExportPlugin", "~1.5.0");
    if (export_dep.isOk()) {
        deps.push_back(export_dep.unwrap());
    }

    // 4. 软依赖：日志查看器
    auto logger_dep = PluginDependency::soft("LoggerViewerPlugin", ">=1.0.0");
    if (logger_dep.isOk()) {
        deps.push_back(logger_dep.unwrap());
    }

    return deps;
}
```

### 示例 4: 条件依赖（基于配置）

```cpp
[[nodiscard]] std::vector<Plugin::PluginDependency> get_dependencies() const override {
    std::vector<Plugin::PluginDependency> deps;

    // 始终需要基础插件
    deps.push_back(
        PluginDependency::required("BasePlugin", ">=1.0.0").unwrap()
    );

    // 根据插件配置决定是否需要其他依赖
    // 注意：这里需要在 on_load() 中动态检查，而不是在 get_dependencies() 中

    return deps;
}

Result<void, std::string> on_load() override {
    // 检查可选依赖是否可用
    bool use_ui_features = ConfigScope("myplugin").get_or<bool>("enable_ui", false);

    if (use_ui_features) {
        // UI 插件存在且启用
        LOG_INFO("UI features enabled");
    } else {
        LOG_INFO("UI features disabled (optional dependency not loaded)");
    }

    return Result<void, std::string>::ok();
}
```

---

## API 参考

### PluginDependency 结构

```cpp
struct PluginDependency {
    std::string plugin_name;      // 依赖的插件名称
    VersionRange version_range;   // 版本约束
    DependencyType type;          // 依赖类型

    // 检查版本是否满足要求
    [[nodiscard]] bool is_satisfied_by(const Version& version) const;

    // 转换为字符串
    [[nodiscard]] std::string to_string() const;
};
```

### 静态工厂方法

```cpp
// 创建必需依赖
static Result<PluginDependency, std::string> required(
    std::string name,
    std::string version_range
);

// 创建可选依赖
static Result<PluginDependency, std::string> optional(
    std::string name,
    std::string version_range
);

// 创建软依赖
static Result<PluginDependency, std::string> soft(
    std::string name,
    std::string version_range
);
```

### PluginManager 方法

```cpp
class PluginManager {
public:
    // 从配置文件初始化依赖模式
    void initialize_dependency_config();

    // 设置依赖解析模式
    void set_dependency_mode(DependencyResolutionMode mode);

    // 获取依赖解析模式
    [[nodiscard]] DependencyResolutionMode get_dependency_mode() const;

    // 获取最后一次解析结果
    [[nodiscard]] DependencyResolutionResult get_last_resolution_result() const;

    // 解析并加载所有插件
    Result<void, std::string> load_all_with_dependencies();
};
```

### 依赖解析模式

```cpp
enum class DependencyResolutionMode {
    Lenient,    // 宽松模式：禁用不满足依赖的插件，继续加载
    Strict      // 严格模式：遇到依赖错误立即停止
};
```

### 配置文件支持

依赖解析模式可以通过配置文件设置：

**config.json:**
```json
{
  "plugins": {
    "dependency_mode": "lenient"
  }
}
```

**可选值:**
- `"lenient"` - 宽松模式（默认）
- `"strict"` - 严格模式

**在应用中启用配置:**

```cpp
// 应用初始化
auto& pm = PluginManager::instance();

// 从配置文件读取依赖模式
pm.initialize_dependency_config();

// 添加插件
pm.add_builtin(std::make_unique<BuiltinPlugin>());
pm.load_from_directory("plugins");

// 解析并加载所有插件（按依赖顺序）
auto result = pm.load_all_with_dependencies();

if (result.isErr()) {
    LOG_ERROR("Failed to load plugins: {}", result.error());
    return;
}

// 查看解析结果
auto resolution = pm.get_last_resolution_result();
LOG_INFO("Loaded {} plugins", resolution.load_order.size());

for (const auto& entry : resolution.load_order) {
    LOG_INFO("  - {} (order: {})", entry.plugin_name, entry.load_order);
}

// 检查错误
for (const auto& error : resolution.errors) {
    LOG_WARN("Dependency error: {}", error.to_string());
}

// 检查被禁用的插件
for (const auto& name : resolution.disabled_plugins) {
    LOG_WARN("Disabled plugin: {}", name);
}
```

---

## 最佳实践

### ✅ DO - 推荐做法

#### 1. 使用 ^ 操作符（兼容版本）

```cpp
// ✅ 好：允许兼容更新
PluginDependency::required("CorePlugin", "^2.0.0")
// 匹配: 2.0.0, 2.1.0, 2.9.0
// 不匹配: 3.0.0, 1.9.0
```

#### 2. 为主要依赖使用 Required

```cpp
// ✅ 好：核心功能必需
PluginDependency::required("BuiltinPlugin", ">=1.0.0")
```

#### 3. 为可选功能使用 Optional

```cpp
// ✅ 好：UI 增强不是必需的
PluginDependency::optional("UIPlugin", "^1.5.0")
```

#### 4. 记录依赖信息

```cpp
Result<void, std::string> on_load() override {
    LOG_INFO("MyPlugin loading with dependencies...");

    // 检查可选依赖
    // (在运行时检查，不在 get_dependencies() 中)

    return Result<void, std::string>::ok();
}
```

#### 5. 优雅降级

```cpp
Result<void, std::string> on_load() override {
    // 注册基础功能
    ContentRegistry::Commands::add("myplugin.basic", ...);

    // 如果可选依赖可用，注册增强功能
    // (通过检查插件是否已加载)

    return Result<void, std::string>::ok();
}
```

### ❌ DON'T - 避免的做法

#### 1. 不要过度限制版本

```cpp
// ❌ 不好：太严格
PluginDependency::required("Plugin", "1.2.3")
// 只允许 1.2.3，即使是 bug 修复版本 1.2.4 也不行

// ✅ 好：使用兼容版本
PluginDependency::required("Plugin", "^1.2.3")
// 允许 1.2.3 到 2.0.0 之前的任何版本
```

#### 2. 不要创建循环依赖

```cpp
// ❌ 糟糕：循环依赖
// PluginA 依赖 PluginB
// PluginB 依赖 PluginA
// 这会导致加载失败！

// ✅ 解决方案：提取共同依赖到 PluginC
// PluginA 依赖 PluginC
// PluginB 依赖 PluginC
```

#### 3. 不要在 get_dependencies() 中执行复杂逻辑

```cpp
// ❌ 不好：在 get_dependencies() 中执行复杂操作
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    // 读取文件
    // 网络请求
    // 复杂计算
    // ...这些都不应该在这里！
}

// ✅ 好：只返回静态依赖列表
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {
        PluginDependency::required("Plugin", "^1.0.0").unwrap()
    };
}
```

#### 4. 不要忽略版本验证

```cpp
// ❌ 不好：不指定版本
// (可能加载不兼容的版本)

// ✅ 好：指定版本要求
PluginDependency::required("Plugin", "^2.0.0")
```

---

## 故障排除

### 问题 1: 插件无法加载

**症状**: 插件显示在日志中，但未被加载

**可能原因**:
- 依赖的插件不存在
- 依赖的插件版本不满足要求
- 循环依赖

**解决方案**:
```cpp
// 查看解析结果
auto resolution = pm.get_last_resolution_result();

// 检查错误
for (const auto& error : resolution.errors) {
    LOG_ERROR("Dependency error: {}", error.to_string());
}

// 检查被禁用的插件
for (const auto& name : resolution.disabled_plugins) {
    LOG_WARN("Disabled: {}", name);
}
```

### 问题 2: 版本冲突

**症状**: 日志显示 "Version conflict"

**示例输出**:
```
[ERROR] Version conflict:
  PluginA (1.0.0) requires PluginB ^2.0.0
  PluginB (1.5.0) is installed
```

**解决方案**:
- 升级 PluginB 到 2.0.0+
- 或降低 PluginA 的版本要求
- 或修改 PluginA 以兼容 PluginB 1.5.0

### 问题 3: 循环依赖

**症状**: 日志显示 "Circular dependency detected"

**示例输出**:
```
[ERROR] Circular dependency detected:
  PluginA -> PluginB -> PluginC -> PluginA
```

**解决方案**:
重构插件以移除循环：
```
修改前:
  PluginA → PluginB → PluginC → PluginA  ❌

修改后:
  PluginA → PluginC ✅
  PluginB → PluginC ✅
```

### 问题 4: get_dependencies() 返回错误

**症状**: 版本范围解析失败

**示例输出**:
```
[ERROR] Failed to parse version range: ">=1.0.0 <2.0"
Invalid version specification
```

**解决方案**:
检查版本范围语法，使用正确的空格分隔：
```cpp
// ❌ 错误
">=1.0.0<2.0.0"  // 缺少空格

// ✅ 正确
">=1.0.0 <2.0.0"  // 有空格
```

### 调试技巧

#### 1. 启用详细日志

```cpp
// 在应用初始化时
Logger::set_level(LogLevel::Debug);
```

#### 2. 可视化依赖图

```cpp
// 生成 DOT 格式依赖图
auto dot_graph = DependencyResolver::visualize_dependency_graph(plugins);
LOG_INFO("Dependency graph:\n{}", dot_graph);

// 使用 Graphviz 可视化
// 复制输出到 https://dreampuf.github.io/GraphvizOnline/
```

#### 3. 检查加载顺序

```cpp
auto resolution = pm.get_last_resolution_result();
for (const auto& entry : resolution.load_order) {
    LOG_INFO("{}. {} depends on: {}",
             entry.load_order,
             entry.plugin_name,
             entry.dependencies.size());
}
```

---

## 版本演进示例

### 场景：插件 API 演进

**V1.0.0** - 初始版本
```cpp
// 没有依赖
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {};
}
```

**V2.0.0** - 添加核心依赖
```cpp
// 需要 CorePlugin 2.0+
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {
        PluginDependency::required("CorePlugin", "^2.0.0").unwrap()
    };
}
```

**V3.0.0** - 添加可选功能
```cpp
// 核心 + 可选 UI 增强
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {
        PluginDependency::required("CorePlugin", "^2.0.0").unwrap(),
        PluginDependency::optional("UIPlugin", "^1.5.0").unwrap()
    };
}
```

---

## 常见用例

### 用例 1: 功能模块化

```cpp
class DataProcessorPlugin : public IPlugin {
public:
    [[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
        return {
            // 需要基础数据处理
            PluginDependency::required("DataFoundation", "^1.0.0").unwrap(),

            // 可选：高级算法
            PluginDependency::optional("AdvancedAlgorithms", "~2.0.0").unwrap(),

            // 可选：导出支持
            PluginDependency::optional("DataExport", ">=3.0.0").unwrap()
        };
    }
};
```

### 用例 2: UI 插件

```cpp
class AdvancedUIPlugin : public IPlugin {
public:
    [[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
        return {
            // 必需：核心 UI 系统
            PluginDependency::required("BuiltinUI", "^1.0.0").unwrap(),

            // 可选：主题系统
            PluginDependency::optional("ThemeManager", "^2.0.0").unwrap(),

            // 软依赖：日志查看器
            PluginDependency::soft("LoggerViewer", ">=1.0.0").unwrap()
        };
    }
};
```

### 用例 3: 媒体处理

```cpp
class VideoEditorPlugin : public IPlugin {
public:
    [[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
        return {
            // 必需：FFmpeg
            PluginDependency::required("FFmpegPlugin", "^4.0.0").unwrap(),

            // 可选：硬件加速
            PluginDependency::optional("GPUAcceleration", ">=2.0.0").unwrap(),

            // 可选：编解码器
            PluginDependency::optional("ExtraCodecs", ">=1.0.0").unwrap()
        };
    }
};
```

---

## 总结

DearTs Framework 插件依赖管理系统提供了强大而灵活的工具来管理插件间的依赖关系。

### 核心要点

1. **声明依赖**: 在 `get_dependencies()` 中返回依赖列表
2. **选择类型**: Required（必需）、Optional（可选）、Soft（软依赖）
3. **指定版本**: 使用 `^` 操作符允许兼容更新
4. **避免循环**: 确保依赖图是无环的
5. **优雅降级**: 在 `on_load()` 中处理可选依赖

### 快速参考

```cpp
// 1. 声明依赖
[[nodiscard]] std::vector<PluginDependency> get_dependencies() const override {
    return {
        PluginDependency::required("Core", "^1.0.0").unwrap(),
        PluginDependency::optional("Enhanced", "~2.0.0").unwrap(),
        PluginDependency::soft("Logger", ">=1.0.0").unwrap()
    };
}

// 2. 初始化配置（可选）
pm.initialize_dependency_config();

// 3. 解析并加载
pm.set_dependency_mode(DependencyResolutionMode::Lenient);
pm.load_all_with_dependencies();

// 4. 检查结果
auto resolution = pm.get_last_resolution_result();
```

### 相关文档

- [插件开发快速指南](../../QUICKSTART.md)
- [插件系统 API 参考](../../dearts-dev/references/plugin_system_api.md)
- [ContentRegistry 文档](../../docs/plugin_system_guide.md)

---

**文档版本**: 1.0.0
**最后更新**: 2025-01-03
**维护者**: DearTs Team
