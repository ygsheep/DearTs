# 插件系统

DearTs Framework 提供功能完整的插件系统，支持动态加载、依赖管理、生命周期管理和版本控制。

## 核心特性

- ✅ **插件依赖管理** - 三种依赖类型（必需/可选/软依赖）
- ✅ **语义化版本控制** - SemVer 2.0.0 + npm 风格版本范围
- ✅ **生命周期管理** - 加载、卸载、启用、禁用
- ✅ **类型安全** - 使用 Result<T, E> 进行错误处理
- ✅ **动态加载** - 支持从动态库加载插件
- ✅ **事件驱动** - 与 Content Registry、EventBus 无缝集成
- ✅ **API 版本检查** - 确保插件与框架兼容

## 快速开始

### 创建基础插件

```cpp
#include "core/plugin/plugin.h"

class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
            .description = "My awesome plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令、视图等
        return Result::ok();
    }
};

// 注册插件
PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);
```

### 创建带依赖的插件

```cpp
class AdvancedPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "AdvancedPlugin",
            .author = "Your Name",
            .description = "Plugin with dependencies",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 声明依赖
    std::vector<PluginDependency> get_dependencies() const override {
        return {
            PluginDependency::required("CorePlugin", ">=2.0.0").unwrap(),
            PluginDependency::optional("UIPlugin", "^1.5.0").unwrap(),
            PluginDependency::soft("AnalyticsPlugin", "~1.2.0").unwrap()
        };
    }

    Result<void, std::string> on_load() override {
        // 插件加载逻辑
        return Result::ok();
    }
};
```

## 插件生命周期

```
未加载 (Unloaded)
    ↓ add_builtin() / load_from_file()
已加载 (Loaded)
    ↓ enable()
已启用 (Enabled)
    ↓ disable()
已加载 (Loaded)
    ↓ unload()
未加载 (Unloaded)
```

## 依赖类型

| 类型 | 说明 | 缺失时行为 |
|------|------|------------|
| `Required` | 必需依赖 | 加载失败 |
| `Optional` | 可选依赖 | 警告，继续加载 |
| `Soft` | 软依赖 | 静默忽略 |

## 版本范围语法

| 语法 | 说明 | 示例 |
|------|------|------|
| `1.2.3` | 精确版本 | 只匹配 1.2.3 |
| `^1.2.3` | 兼容版本 | >=1.2.3 <2.0.0 |
| `~1.2.3` | 补丁级更新 | >=1.2.3 <1.3.0 |
| `>=1.2.3` | 大于等于 | 1.2.3 及以上 |
| `1.2.*` | 通配符 | >=1.2.0 <1.3.0 |

## PluginManager API

```cpp
// 单例访问
PluginManager::instance()

// 插件管理
add_builtin(plugin)
load_from_file(path)
load_from_directory(path)
unload(name)
enable(name)
disable(name)
reload(name)

// 查询
get_plugin(name)
get_all_plugins_info()
get_plugin_state(name)
is_plugin_builtin(name)

// 依赖管理
set_dependency_mode(mode)
load_all_with_dependencies()
get_last_resolution_result()
```

## 相关文档

详细的 API 文档请参考：
- **[plugin_system_api.md](plugin_system_api.md)** - 完整 API 手册
- **[plugin_system_guide.md](../../docs/plugin_system_guide.md)** - 用户指南
- **[QUICKSTART.md](../../plugins/QUICKSTART.md)** - 插件快速开始

## 头文件

```cpp
#include "core/plugin/plugin.h"              // 核心插件系统
#include "core/plugin/plugin_dependency.h"   // 依赖管理
#include "core/plugin/version.h"             // 版本控制
#include "core/plugin/version_range.h"       // 版本范围
```
