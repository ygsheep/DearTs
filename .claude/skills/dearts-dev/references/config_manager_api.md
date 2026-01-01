# ConfigManager API 完全手册

## 概述

ConfigManager 是 DearTs Framework 的配置管理系统，提供类型安全、线程安全的配置管理接口。

**核心特性：**
- ✅ **类型安全** - 支持 bool, int, double, string
- ✅ **层级键** - 点号分隔的配置路径（如 `app.window.width`）
- ✅ **元数据** - 描述、默认值、验证器
- ✅ **变更回调** - 配置变更时触发
- ✅ **线程安全** - 使用 mutex 保护
- ✅ **RAII** - ConfigScope 自动管理作用域

---

## 快速开始

### 基本用法

```cpp
#include "core/config/config_manager.h"

using namespace DearTs::Core::Config;

// 1. 设置配置值
ConfigManager::instance().set("app.window.width", 1280);
ConfigManager::instance().set("app.window.height", 720);
ConfigManager::instance().set("app.theme", "Dark");

// 2. 获取配置值
auto width = ConfigManager::instance().get<int>("app.window.width");
if (width.isOk()) {
    int w = width.unwrap();
    LOG_INFO("Window width: {}", w);
}

// 3. 使用默认值
int height = ConfigManager::instance().get_or("app.window.height", 720);
```

---

## API 参考

### 1. 获取单例

```cpp
ConfigManager& ConfigManager::instance()
```

**示例：**
```cpp
auto& config = ConfigManager::instance();
```

---

### 2. 设置配置值

```cpp
template<typename T>
Result<void, std::string> set(const std::string& key, T value)
```

**参数：**
- `key` - 配置键（支持点号分隔，如 `"app.window.width"`）
- `value` - 配置值（bool, int, double, string）

**返回：**
- 成功：`Result<void, std::string>::ok()`
- 失败：`Result<void, std::string>::err("错误信息")`

**示例：**

```cpp
// 设置整数
auto result = ConfigManager::instance().set("app.window.width", 1280);
if (result.isErr()) {
    LOG_ERROR("Failed to set width: {}", result.error());
}

// 设置布尔值
ConfigManager::instance().set("app.auto_save", true);

// 设置浮点数
ConfigManager::instance().set("app.scale", 1.5);

// 设置字符串
ConfigManager::instance().set("app.theme", "Dark");
```

---

### 3. 获取配置值

```cpp
template<typename T>
Result<T, std::string> get(const std::string& key, T default_value = T{}) const
```

**参数：**
- `key` - 配置键
- `default_value` - 默认值（可选）

**返回：**
- 成功：`Result<T, std::string>::ok(value)`
- 失败：`Result<T, std::string>::err("错误信息")`

**示例：**

```cpp
// 方法 1：使用 Result 类型
auto width_result = ConfigManager::instance().get<int>("app.window.width");
if (width_result.isOk()) {
    int width = width_result.unwrap();
    LOG_INFO("Width: {}", width);
} else {
    LOG_ERROR("Error: {}", width_result.error());
}

// 方法 2：使用默认值
int width = ConfigManager::instance()
    .get_or("app.window.width", 1280);

// 方法 3：链式调用（高级）
auto width = ConfigManager::instance()
    .get<int>("app.window.width")
    .map([](int w) { return w * 2; })  // 乘以 2
    .unwrap_or(1280);                  // 失败时使用默认值
```

---

### 4. 检查配置是否存在

```cpp
bool has(const std::string& key) const
```

**示例：**
```cpp
if (ConfigManager::instance().has("app.window.width")) {
    LOG_INFO("Width is set");
}
```

---

### 5. 删除配置

```cpp
void remove(const std::string& key)
```

**示例：**
```cpp
ConfigManager::instance().remove("app.temp_config");
```

---

### 6. 注册元数据

```cpp
void register_meta(const std::string& key, ConfigMeta meta)
```

**ConfigMeta 结构：**
```cpp
struct ConfigMeta {
    std::string description;              // 描述
    ConfigValue default_value;            // 默认值
    bool is_required;                     // 是否必需
    ValidateCallback validate_callback;   // 验证回调
    ChangeCallback change_callback;       // 变更回调
};
```

**示例：**

```cpp
// 注册带元数据的配置
ConfigManager::instance().register_meta("app.window.width", {
    .description = "Window width in pixels",
    .default_value = 1280,
    .is_required = false,
    .validate_callback = [](const ConfigValue& value) -> Result<void, std::string> {
        if (std::holds_alternative<int>(value)) {
            int w = std::get<int>(value);
            if (w < 640) {
                return Result::err("Width must be at least 640");
            }
            if (w > 7680) {
                return Result::err("Width must not exceed 7680");
            }
        }
        return Result::ok();
    },
    .change_callback = [](const ConfigValue& value) {
        LOG_INFO("Window width changed to {}", std::get<int>(value));
    }
});

// 使用配置（自动应用验证和回调）
ConfigManager::instance().set("app.window.width", 1920);
```

---

### 7. 添加全局变更回调

```cpp
void add_change_callback(
    std::function<void(const std::string&, const ConfigValue&, const ConfigValue&)> callback
)
```

**参数：**
- `callback` - 回调函数，参数为 (key, old_value, new_value)

**示例：**

```cpp
// 监听所有配置变更
ConfigManager::instance().add_change_callback(
    [](const std::string& key, const ConfigValue& old_val, const ConfigValue& new_val) {
        LOG_INFO("Config changed: {}", key);

        // 根据类型打印
        if (std::holds_alternative<int>(new_val)) {
            LOG_INFO("  Old: {}, New: {}", std::get<int>(old_val), std::get<int>(new_val));
        } else if (std::holds_alternative<std::string>(new_val)) {
            LOG_INFO("  Old: {}, New: {}",
                std::get<std::string>(old_val),
                std::get<std::string>(new_val));
        }
    }
);
```

---

### 8. 获取所有配置键

```cpp
std::vector<std::string> get_all_keys() const
```

**示例：**
```cpp
auto keys = ConfigManager::instance().get_all_keys();
for (const auto& key : keys) {
    LOG_INFO("Config: {}", key);
}
```

---

### 9. 清空所有配置

```cpp
void clear()
```

**示例：**
```cpp
ConfigManager::instance().clear();
```

---

## ConfigScope - RAII 作用域管理

### 概述

ConfigScope 提供自动前缀管理，避免重复编写配置前缀。

### 基本用法

```cpp
// 创建作用域
ConfigScope scope("app.window");

// 设置配置（自动添加前缀）
scope.set("width", 1280);   // 实际键: "app.window.width"
scope.set("height", 720);   // 实际键: "app.window.height"

// 获取配置
int width = scope.get_or<int>("width", 1280);   // 自动添加前缀
```

### 构造函数

```cpp
explicit ConfigScope(const std::string& prefix)
```

**示例：**
```cpp
// 方法 1：直接构造
{
    ConfigScope scope("app.window");
    scope.set("width", 1280);
}  // scope 离开作用域自动销毁

// 方法 2：类成员
class MyClass {
private:
    ConfigScope m_config_scope{"app.window"};

public:
    void set_window_size(int w, int h) {
        m_config_scope.set("width", w);
        m_config_scope.set("height", h);
    }
};
```

### 实用方法

```cpp
// 获取完整键名
std::string make_key(const std::string& key) const

// 设置值
template<typename T>
Result<void, std::string> set(const std::string& key, T value)

// 获取值
template<typename T>
Result<T, std::string> get(const std::string& key, T default_value = T{}) const

// 获取值（带默认）
template<typename T>
T get_or(const std::string& key, T default_value) const
```

---

## 实际应用示例

### 示例 1：应用配置

```cpp
class AppConfig {
public:
    static void init() {
        auto& config = ConfigManager::instance();

        // 注册所有应用配置
        config.register_meta("app.window.width", {
            .description = "Window width",
            .default_value = 1280,
            .is_required = false
        });

        config.register_meta("app.window.height", {
            .description = "Window height",
            .default_value = 720,
            .is_required = false
        });

        config.register_meta("app.window.maximized", {
            .description = "Window maximized state",
            .default_value = false,
            .is_required = false
        });

        config.register_meta("app.theme", {
            .description = "UI theme",
            .default_value = std::string("Dark"),
            .is_required = false,
            .validate_callback = [](const ConfigValue& value) {
                if (std::holds_alternative<std::string>(value)) {
                    std::string theme = std::get<std::string>(value);
                    if (theme != "Light" && theme != "Dark" && theme != "Auto") {
                        return Result<void, std::string>::err(
                            "Invalid theme. Must be Light, Dark, or Auto"
                        );
                    }
                }
                return Result::ok();
            }
        });

        // 加载配置文件
        config.load_from_file("config.json");
    }

    static void save() {
        ConfigManager::instance().save_to_file("config.json");
    }
};
```

### 示例 2：视图配置

```cpp
class HexEditorView : public View {
private:
    ConfigScope m_config{"hex.editor"};

public:
    void draw_content() override {
        // 读取配置
        bool show_offsets = m_config.get_or<bool>("show_offsets", true);
        int bytes_per_row = m_config.get_or<int>("bytes_per_row", 16);

        // 绘制...

        // 右键菜单
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Show Offsets", nullptr, show_offsets)) {
                m_config.set("show_offsets", !show_offsets);
            }
            ImGui::EndPopup();
        }
    }
};
```

### 示例 3：插件配置

```cpp
class MyPlugin : public IPlugin {
private:
    ConfigScope m_config{"plugins.myplugin"};

public:
    Result<void, std::string> on_load() override {
        // 注册配置
        ConfigManager::instance().register_meta("plugins.myplugin.auto_save", {
            .description = "Auto save data",
            .default_value = true,
            .is_required = false,
            .change_callback = [](const ConfigValue& value) {
                LOG_INFO("Auto save: {}", std::get<bool>(value));
            }
        });

        // 读取配置
        bool auto_save = m_config.get_or<bool>("auto_save", true);
        if (auto_save) {
            // 启用自动保存
        }

        return Result::ok();
    }
};
```

### 示例 4：实时配置更新

```cpp
class SettingsView : public View {
private:
    bool m_auto_save = false;
    int m_save_interval = 60;

public:
    void draw_content() override {
        auto& config = ConfigManager::instance();

        // 自动保存
        bool auto_save = config.get_or<bool>("app.auto_save", false);
        if (ImGui::Checkbox("Auto Save", &auto_save)) {
            config.set("app.auto_save", auto_save);
        }

        // 保存间隔
        int interval = config.get_or<int>("app.save_interval", 60);
        if (ImGui::SliderInt("Save Interval (seconds)", &interval, 10, 600)) {
            config.set("app.save_interval", interval);
        }

        // 主题选择
        std::string theme = config.get_or<std::string>("app.theme", "Dark");
        const char* themes[] = {"Light", "Dark", "Auto"};
        int current = (theme == "Light" ? 0 : (theme == "Dark" ? 1 : 2));

        if (ImGui::Combo("Theme", &current, themes, 3)) {
            config.set("app.theme", std::string(themes[current]));
        }
    }
};
```

---

## 最佳实践

### ✅ DO

1. **使用 ConfigScope 管理前缀**
   ```cpp
   // ✅ 好
   ConfigScope scope{"app.window"};
   scope.set("width", 1280);

   // ❌ 不好
   ConfigManager::instance().set("app.window.width", 1280);
   ```

2. **注册元数据**
   ```cpp
   // ✅ 好 - 提供文档和验证
   config.register_meta("app.window.width", {
       .description = "Window width",
       .default_value = 1280,
       .is_required = false
   });
   ```

3. **使用 Result 类型处理错误**
   ```cpp
   // ✅ 好
   auto result = config.set("key", value);
   if (result.isErr()) {
       LOG_ERROR("Failed: {}", result.error());
   }

   // ❌ 不好 - 忽略错误
   config.set("key", value);
   ```

4. **使用变更回调**
   ```cpp
   // ✅ 好 - 自动响应配置变更
   config.register_meta("app.theme", {
       .change_callback = [](const ConfigValue& value) {
           apply_theme(std::get<std::string>(value));
       }
   });
   ```

### ❌ DON'T

1. **不要硬编码配置**
   ```cpp
   // ❌ 不好
   int window_width = 1280;

   // ✅ 好
   int window_width = config.get_or("app.window.width", 1280);
   ```

2. **不要重复前缀**
   ```cpp
   // ❌ 不好
   config.set("app.window.width", 1280);
   config.set("app.window.height", 720);
   config.set("app.window.title", "My App");

   // ✅ 好
   ConfigScope scope{"app.window"};
   scope.set("width", 1280);
   scope.set("height", 720);
   scope.set("title", "My App");
   ```

3. **不要忽略类型安全**
   ```cpp
   // ❌ 不好 - 类型可能不匹配
   auto value = config.get("app.width");
   int width = std::get<int>(value);

   // ✅ 好 - 类型安全
   auto result = config.get<int>("app.width");
   if (result.isOk()) {
       int width = result.unwrap();
   }
   ```

---

## 完整示例项目

```cpp
#include "core/config/config_manager.h"
#include "liblogger/logger.h"

class MyApp : public Application {
private:
    ConfigScope m_config{"app"};

public:
    bool on_init() override {
        // 初始化配置
        init_config();

        // 加载配置
        auto& config = ConfigManager::instance();
        config.load_from_file("config.json");

        // 监听配置变更
        config.add_change_callback([](const std::string& key,
                                       const auto& old_val,
                                       const auto& new_val) {
            LOG_INFO("Config changed: {}", key);
        });

        return true;
    }

    void on_shutdown() override {
        // 保存配置
        ConfigManager::instance().save_to_file("config.json");
    }

    void on_render() override {
        // 使用配置
        int width = m_config.get_or<int>("window.width", 1280);
        int height = m_config.get_or<int>("window.height", 720);
        std::string theme = m_config.get_or<std::string>("theme", "Dark");

        // 渲染 UI...
    }

private:
    void init_config() {
        auto& config = ConfigManager::instance();

        // 窗口配置
        config.register_meta("app.window.width", {
            .description = "Window width",
            .default_value = 1280,
            .is_required = false
        });

        config.register_meta("app.window.height", {
            .description = "Window height",
            .default_value = 720,
            .is_required = false
        });

        config.register_meta("app.theme", {
            .description = "UI theme",
            .default_value = std::string("Dark"),
            .is_required = false
        });

        config.register_meta("app.auto_save", {
            .description = "Auto save",
            .default_value = true,
            .is_required = false
        });
    }
};
```

---

## API 快速参考

| 方法 | 说明 |
|------|------|
| `instance()` | 获取单例 |
| `set<T>(key, value)` | 设置配置值 |
| `get<T>(key, default)` | 获取配置值（Result） |
| `get_or<T>(key, default)` | 获取配置值（直接返回） |
| `has(key)` | 检查键是否存在 |
| `remove(key)` | 删除配置 |
| `register_meta(key, meta)` | 注册元数据 |
| `get_meta(key)` | 获取元数据 |
| `add_change_callback(cb)` | 添加变更回调 |
| `load_from_file(path)` | 从文件加载 |
| `save_to_file(path)` | 保存到文件 |
| `get_all_keys()` | 获取所有键 |
| `clear()` | 清空所有配置 |

---

**文件**: `core/config/config_manager.h`
**源码**: `core/config/config_manager.cpp`
**相关**: Result 类型, 日志系统
