# DearTs Framework - Claude Code Skills 使用指南

> 使用 Claude Code Skills 为 DearTs 项目创建专业代码和文档

---

## 目录

- [Claude Code Skills 概述](#claude-code-skills-概述)
- [DearTs Skills 集合](#dearts-skills-集合)
- [安装和使用](#安装和使用)
- [实战示例](#实战示例)
- [最佳实践](#最佳实践)

---

## Claude Code Skills 概述

Claude Code Skills 是一个模块化的技能系统，允许将专业知识、工作流程和工具打包成可重用的技能包。DearTs Framework 提供了四个专业 Skills 来加速开发。

### 什么是 Skills？

Skills 是为 Claude Code AI 助手设计的"专业领域知识包"，每个 Skill 包含：
- **专业知识** - 特定领域的最佳实践和模式
- **工作流程** - 多步骤任务的标准化流程
- **代码模板** - 可重用的代码片段和模式
- **工具集成** - 特定工具和框架的使用说明

### DearTs Skills 的优势

✅ **一致性** - 所有代码遵循相同的命名规范和架构模式
✅ **效率** - 快速生成样板代码，专注于业务逻辑
✅ **质量** - 遵循最佳实践，减少常见错误
✅ **文档化** - 自动生成文档和注释
✅ **标准化** - 团队成员使用统一的代码风格

---

## DearTs Skills 集合

DearTs Framework 提供以下四个专业 Skills：

### 1. dearts-module-generator

**用途**：生成新的 core 模块

**功能**：
- 生成符合 DearTs 规范的头文件和源文件
- 自动应用命名约定（snake_case、m_ 前缀等）
- 包含完整的 Doxygen 文档注释
- 生成 CMakeLists.txt 集成代码

**使用场景**：
- 创建新的核心模块（input、render、audio 等）
- 扩展框架功能
- 添加新的子系统

**示例**：
```
用户: "创建一个输入管理器模块"
Claude: 生成 core/input/input_manager.h 和 .cpp
```

---

### 2. dearts-app-generator

**用途**：生成完整的 DearTs 应用程序

**功能**：
- 生成 main.cpp 入口文件
- 创建 Application 子类
- 配置 CMakeLists.txt
- 包含完整的生命周期方法
- 提供多种应用类型模板

**使用场景**：
- 创建新应用程序
- 添加示例程序到 examples/
- 快速原型开发

**示例**：
```
用户: "创建一个简单的绘图应用"
Claude: 生成完整的绘图应用程序代码
```

---

### 3. dearts-event-system

**用途**：EventBus 事件系统使用指南

**功能**：
- 类型安全的事件定义
- 订阅/发布模式
- 同步和异步事件处理
- RAII 风格的事件管理
- 线程安全的事件处理

**使用场景**：
- 实现模块间通信
- 处理用户输入事件
- 构建事件驱动架构
- 解耦系统组件

**示例**：
```
用户: "如何使用 EventBus 处理玩家移动事件？"
Claude: 提供完整的事件系统代码示例
```

---

### 4. dearts-documentation

**用途**：生成项目文档

**功能**：
- API 文档生成
- 架构文档编写
- 教程和指南创建
- README.md 生成
- 代码文档注释

**使用场景**：
- 为模块编写文档
- 创建教程
- 生成 API 参考
- 编写架构说明

**示例**：
```
用户: "为新模块生成 API 文档"
Claude: 生成完整的 Markdown 文档
```

---

## 安装和使用

### 安装步骤

DearTs Skills 已包含在项目中，位于 `.claude-skills/` 目录：

```
DearTs/
├── .claude-skills/
│   ├── dearts-module-generator/
│   │   └── SKILL.md
│   ├── dearts-app-generator/
│   │   └── SKILL.md
│   ├── dearts-event-system/
│   │   └── SKILL.md
│   └── dearts-documentation/
│       └── SKILL.md
```

### 配置 Claude Code

确保 Claude Code 能够识别这些 Skills：

1. **Claude Code 配置**：Skills 通过描述和关键词自动触发
2. **项目关联**：在 DearTs 项目目录中使用 Claude Code

### 触发 Skills

Skills 根据用户的请求自动触发：

```
# 触发 dearts-module-generator
"创建一个新的渲染模块"
"添加一个音频管理器到 core/"

# 触发 dearts-app-generator
"创建一个游戏示例程序"
"生成一个简单的测试应用"

# 触发 dearts-event-system
"如何订阅事件？"
"实现玩家跳跃事件处理"

# 触发 dearts-documentation
"为这个模块生成文档"
"创建 API 文档"
```

---

## 实战示例

### 示例 1：创建输入管理器模块

**用户请求**：
```
创建一个输入管理器模块，支持键盘和鼠标输入
```

**Claude 使用 dearts-module-generator**：

1. **理解需求**：
   - 模块名：input_manager
   - 功能：键盘状态追踪、鼠标位置获取
   - 类名：InputManager

2. **生成文件**：
   - `core/input/input_manager.h`
   - `core/input/input_manager.cpp`

3. **更新 CMakeLists.txt**

4. **提供使用示例**

**生成的代码**：

```cpp
// core/input/input_manager.h
#pragma once

#include <SDL3/SDL.h>

namespace DearTs {
namespace Core::Input {

class InputManager {
public:
    static InputManager& get_instance();

    bool is_key_pressed(SDL_KeyCode key) const;
    void get_mouse_position(int* x, int* y) const;

private:
    InputManager();
    ~InputManager() = default;

    SDL_KeyboardState m_keyboard_state;
    int m_mouse_x = 0;
    int m_mouse_y = 0;
};

} // namespace DearTs::Core::Input
} // namespace DearTs
```

---

### 示例 2：创建游戏应用

**用户请求**：
```
创建一个简单的游戏应用，显示玩家可以移动的方块
```

**Claude 使用 dearts-app-generator**：

1. **创建应用结构**：
   - `examples/game_demo/main.cpp`
   - `examples/game_demo/CMakeLists.txt`

2. **实现游戏逻辑**：
   - Player 类
   - 渲染循环
   - 输入处理

**生成的代码**：

```cpp
#include "core/app/application.h"
#include "core/input/input_manager.h"

class GameDemo : public Application {
protected:
    bool on_init() override {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        m_player_x = 640.0f;
        m_player_y = 360.0f;
        return true;
    }

    void on_update(double delta_time) override {
        auto& input = InputManager::get_instance();

        float speed = 200.0f * delta_time;
        if (input.is_key_pressed(SDLK_W)) m_player_y -= speed;
        if (input.is_key_pressed(SDLK_S)) m_player_y += speed;
        if (input.is_key_pressed(SDLK_A)) m_player_x -= speed;
        if (input.is_key_pressed(SDLK_D)) m_player_x += speed;
    }

    void on_render() override {
        SDL_RenderClear(m_renderer);

        SDL_FRect player = {m_player_x, m_player_y, 50, 50};
        SDL_SetRenderDrawColor(m_renderer, 255, 100, 100, 255);
        SDL_RenderFillRect(m_renderer, &player);

        SDL_RenderPresent(m_renderer);
    }

private:
    float m_player_x, m_player_y;
};

int main() {
    ApplicationConfig config;
    config.name = "Game Demo";
    config.window_width = 1280;
    config.window_height = 720;

    auto app = std::make_unique<GameDemo>();
    if (!app->initialize(config)) return -1;
    return app->run();
}
```

---

### 示例 3：实现事件系统

**用户请求**：
```
使用 EventBus 实现玩家得分系统
```

**Claude 使用 dearts-event-system**：

1. **定义事件**：
```cpp
struct ScoreChangeEvent {
    int old_score;
    int new_score;
};
```

2. **发布事件**：
```cpp
void add_score(int points) {
    int old_score = m_score;
    m_score += points;
    EventBus::publish(ScoreChangeEvent{old_score, m_score});
}
```

3. **订阅事件**：
```cpp
m_guard = EventBus::subscribe<ScoreChangeEvent>(
    [](const ScoreChangeEvent& e) {
        LOG_INFO("Score: {} → {}", e.old_score, e.new_score);
        update_ui(e.new_score);
    }
);
```

---

### 示例 4：生成文档

**用户请求**：
```
为 InputManager 模块生成 API 文档
```

**Claude 使用 dearts-documentation**：

生成 `docs/api/input_manager.md`：

```markdown
# InputManager

## Overview

InputManager provides centralized access to keyboard and mouse input
state in a DearTs application.

## Class: InputManager

Singleton class for managing input state.

### Public Methods

#### `static InputManager& get_instance()`

Returns the singleton instance.

**Returns:** Reference to InputManager instance

#### `bool is_key_pressed(SDL_KeyCode key) const`

Check if a key is currently pressed.

**Parameters:**
- `key`: SDL key code to check

**Returns:** true if key is pressed, false otherwise

**Example:**
```cpp
auto& input = InputManager::get_instance();
if (input.is_key_pressed(SDLK_SPACE)) {
    // Handle space key
}
```

## Usage Example

```cpp
void on_update(double delta_time) override {
    auto& input = InputManager::get_instance();

    if (input.is_key_pressed(SDLK_W)) {
        player.move_forward(delta_time);
    }
}
```

## Thread Safety

All methods are thread-safe and can be called from any thread.

## See Also

- [SDL3 Key Codes](https://wiki.libsdl.org/SDL3/SDL_Keycode)
- [Event System Guide](../event_system.md)
```

---

## 最佳实践

### 1. 从简单开始

✅ **推荐**：
```
"创建一个基础的应用程序窗口"
"生成一个简单的模块模板"
```

❌ **避免**：
```
"创建一个完整的 3D 游戏引擎"  # 太复杂
"实现所有功能"                # 不具体
```

### 2. 提供清晰的上下文

✅ **推荐**：
```
"创建一个音频管理器模块，支持播放音乐和音效"
"生成一个事件系统示例，展示玩家跳跃事件的订阅和发布"
```

❌ **避免**：
```
"创建音频模块"              # 缺少细节
"生成事件示例"              # 不明确
```

### 3. 迭代改进

**工作流程**：

1. **生成基础版本**：
   ```
   "创建一个输入管理器"
   ```

2. **添加功能**：
   ```
   "为 InputManager 添加手柄支持"
   ```

3. **完善代码**：
   ```
   "为 InputManager 添加输入映射功能"
   ```

### 4. 组合使用 Skills

**场景**：创建一个完整的游戏系统

```bash
# Step 1: 创建应用
"创建一个游戏应用程序"

# Step 2: 添加模块
"创建一个粒子系统模块"
"创建一个碰撞检测模块"

# Step 3: 集成事件
"使用 EventBus 实现游戏事件系统"

# Step 4: 生成文档
"为游戏系统生成完整文档"
```

### 5. 验证生成的代码

✅ **检查项**：
- [ ] 代码编译通过
- [ ] 命名符合 DearTs 规范
- [ ] 包含必要的错误处理
- [ ] 有适当的日志记录
- [ ] 文档注释完整
- [ ] CMakeLists.txt 正确更新

### 6. 自定义生成的代码

生成代码后，根据需求定制：

```cpp
// 生成的代码
class InputManager {
    bool is_key_pressed(SDL_KeyCode key) const;
};

// 定制后的代码
class InputManager {
    bool is_key_pressed(SDL_KeyCode key) const;

    // 添加：输入重映射
    void remap_key(SDL_KeyCode from, SDL_KeyCode to);

    // 添加：输入组合
    bool is_combo_pressed(std::vector<SDL_KeyCode> keys) const;
};
```

---

## Skills 参考

### 快速参考卡

| 任务 | 使用 Skill | 关键词 |
|------|-----------|--------|
| 创建新模块 | dearts-module-generator | "创建模块"、"生成 core/" |
| 创建应用 | dearts-app-generator | "创建应用"、"生成示例" |
| 事件处理 | dearts-event-system | "订阅事件"、"EventBus" |
| 生成文档 | dearts-documentation | "生成文档"、"API 文档" |

### 常用命令模式

```bash
# 模块生成
"创建 <类型> 管理器模块"
"添加 <功能> 系统到 core/"

# 应用生成
"创建 <描述> 应用程序"
"生成 <类型> 示例程序"

# 事件系统
"如何使用 EventBus 处理 <事件>？"
"实现 <场景> 的事件处理"

# 文档生成
"为 <模块> 生成 API 文档"
"创建 <主题> 教程"
```

---

## 故障排除

### Skill 未触发

**问题**：Claude Code 没有使用相关 Skill

**解决方案**：
1. 确保在 DearTs 项目目录中
2. 使用更明确的触发词
3. 明确引用 DearTs Framework

**示例**：
```
❌ "创建一个 C++ 类"
✅ "创建一个 DearTs core 模块"
```

### 生成的代码不符合预期

**问题**：代码需要调整

**解决方案**：
1. 提供更详细的需求
2. 分步骤生成
3. 明确指定规范

**示例**：
```
❌ "创建输入模块"
✅ "创建 InputManager 模块，包含：
   - is_key_pressed() 方法
   - get_mouse_position() 方法
   - 单例模式
   - 线程安全"
```

---

## 进阶使用

### 自定义 Skill 模板

可以根据项目需求定制 Skills：

1. **复制现有 Skill**
2. **修改 SKILL.md**
3. **添加项目特定规范**

### 集成到 CI/CD

```yaml
# .github/workflows/verify.yml
name: Verify Code Standards

on: [push, pull_request]

jobs:
  verify:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Verify Skills Generation
        run: |
          # 使用 Claude Code Skills 生成测试代码
          # 验证代码符合规范
```

---

## 资源和链接

### 官方文档

- [Claude Code Skills 文档](https://platform.claude.com/docs/en/agents-and-tools/agent-skills)
- [DearTs Framework README](../README.md)
- [DearTs 代码规范](./代码规范.md)

### 示例代码

- `examples/` - 完整的应用程序示例
- `core/` - 核心模块实现

### 社区

- [GitHub Issues](https://github.com/your-org/dearts/issues)
- [Discord 服务器](https://discord.gg/your-server)

---

## 贡献

欢迎贡献新的 Skills 或改进现有 Skills！

### 贡献流程

1. **Fork 仓库**
2. **创建 Skill 分支**
3. **测试 Skill**
4. **提交 Pull Request**

### Skill 提交清单

- [ ] SKILL.md 包含完整的元数据
- [ ] 描述清晰且准确
- [ ] 包含使用示例
- [ ] 遵循 DearTs 代码规范
- [ ] 通过安全扫描

---

## 更新日志

### v1.0.0 (2024-12-28)

**新增**：
- ✨ dearts-module-generator - 模块生成器
- ✨ dearts-app-generator - 应用生成器
- ✨ dearts-event-system - 事件系统指南
- ✨ dearts-documentation - 文档生成器

**改进**：
- 📝 完整的使用指南
- 📚 实战示例
- 🔧 最佳实践

---

<div align="center">

**用 ❤️ 打造的 DearTs Framework 开发工具链**

[⬆ 返回顶部](#dearts-framework---claude-code-skills-使用指南)

</div>
