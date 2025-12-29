# Navigation Plugin - 导航插件

## 概述

导航插件为 DearTs 框架提供了一个美观的侧边栏视图，用于管理和切换所有已注册的视图。它遵循 DearTs 框架的最佳实践，提供了完整的视图导航和管理功能。

## 主要功能

### 1. **侧边栏视图 (SidebarView)**
- 自动发现和显示所有已注册的视图
- 实时搜索和筛选功能
- 按分类组织视图（全部、核心、工具、插件、设置）
- 可视化状态指示器（绿色点 = 可见，灰色点 = 隐藏）
- 美观的 UI 设计，符合 DearTs 框架风格

### 2. **工具栏功能**
- **全部显示**：一键打开所有视图
- **全部隐藏**：一键关闭所有视图
- **重置默认**：恢复到默认显示状态

### 3. **搜索和筛选**
- **实时搜索**：输入关键字即时筛选视图
- **分类筛选**：按视图类型快速筛选
- **显示选项**：
  - 显示/隐藏图标
  - 显示/隐藏描述
  - 紧凑模式切换

### 4. **统计信息**
- 显示当前可见视图数量
- 显示总视图数量
- 实时更新

## 使用方法

### 基本使用

1. **启动应用**
   - 应用启动后，侧边栏会自动显示
   - 默认状态为打开

2. **切换视图显示**
   - 在侧边栏中点击视图名称
   - 点击任意视图项即可切换其显示/隐藏状态
   - 状态点会实时更新（绿色 = 可见，灰色 = 隐藏）

3. **搜索视图**
   - 在搜索框中输入关键字
   - 视图列表会实时过滤显示匹配结果
   - 支持不区分大小写搜索

4. **分类筛选**
   - 点击分类按钮（全部、核心、工具、插件、设置）
   - 快速定位特定类型的视图

5. **批量操作**
   - 点击"全部显示"打开所有视图
   - 点击"全部隐藏"关闭所有视图
   - 点击"重置默认"恢复初始状态

### 显示选项

在侧边栏底部可以切换以下选项：

- **显示图标**：切换是否在视图名称前显示图标
- **显示描述**：切换是否显示视图描述信息
- **紧凑模式**：使用更紧凑的布局，节省空间

## 技术特性

### 遵循 DearTs 框架规范

1. **使用 ViewWindow 基类**
   ```cpp
   class SidebarView : public Core::UI::ViewWindow
   ```

2. **实现 draw_content() 方法**
   ```cpp
   void draw_content() override;
   ```

3. **使用 Content Registry**
   ```cpp
   ContentRegistry::Views::add<SidebarView>();
   ```

4. **Result 类型错误处理**
   ```cpp
   Core::Result<void, std::string> on_load() override;
   ```

5. **Logger 日志系统**
   ```cpp
   LOG_INFO("Navigation plugin loading...");
   ```

### UI/UX 特性

1. **美观的样式**
   - 自定义按钮颜色
   - 半透明背景
   - 平滑的悬停效果

2. **可视化状态**
   - 圆形状态指示器
   - 颜色编码（绿色/灰色）
   - 锁定状态显示

3. **响应式布局**
   - 自适应窗口大小
   - 灵活的间距和填充
   - 可配置的显示选项

4. **高性能**
   - 实时搜索无延迟
   - 最小化重绘
   - 智能状态管理

## 文件结构

```
plugins/navigation/
├── include/
│   ├── navigation_plugin.hpp       # 插件主类
│   └── views/
│       └── sidebar_view.hpp        # 侧边栏视图
├── source/
│   ├── navigation_plugin.cpp       # 插件实现
│   └── sidebar_view.cpp            # 侧边栏实现
├── CMakeLists.txt                  # 构建配置
└── README.md                       # 本文档
```

## 集成到主应用

插件已完全集成到主应用中：

### CMakeLists.txt
```cmake
# Navigation 插件
${CMAKE_SOURCE_DIR}/plugins/navigation/include/navigation_plugin.hpp
${CMAKE_SOURCE_DIR}/plugins/navigation/include/views/sidebar_view.hpp
${CMAKE_SOURCE_DIR}/plugins/navigation/source/navigation_plugin.cpp
${CMAKE_SOURCE_DIR}/plugins/navigation/source/sidebar_view.cpp
```

### dearts_application.cpp
```cpp
#include "navigation_plugin.hpp"

// 在 setup_plugins() 中
auto navigation_plugin = std::make_unique<DearTs::Plugins::Navigation::NavigationPlugin>();
auto result = plugin_manager.add_builtin(std::move(navigation_plugin));
```

## 扩展性

### 添加新分类

在 `sidebar_view.hpp` 中添加新的分类：

```cpp
enum class ViewCategory {
    All,
    Core,
    Tools,
    Plugins,
    Settings,
    YourNewCategory  // 添加新分类
};
```

### 自定义视图注册

插件会自动发现所有通过 `ContentRegistry::Views::add()` 注册的视图，无需手动配置。

### 自定义图标

视图通过 `get_icon()` 方法提供图标：

```cpp
class MyView : public Core::UI::ViewWindow {
public:
    MyView() : ViewWindow("MyView", "🎯") {  // 使用 emoji 作为图标
        // ...
    }
};
```

## 最佳实践

1. **视图命名**
   - 使用清晰、简洁的名称
   - 优先使用中文（符合 DearTs 本地化策略）
   - 避免特殊字符

2. **图标选择**
   - 使用 emoji 图标（简单直观）
   - 或使用 FontAwesome 图标（需要字体支持）
   - 保持图标风格一致

3. **默认显示状态**
   - 核心功能视图默认打开
   - 辅助功能视图默认关闭
   - 用户可以通过侧边栏自由切换

## 与其他插件集成

Navigation 插件可以很好地与现有插件配合使用：

### Settings 插件
- 可以通过侧边栏快速切换设置窗口
- 便于临时查看配置项

### LoggerViewer 插件
- 需要时打开日志查看器
- 不需要时隐藏以节省空间
- 实时查看应用状态

### Builtin 插件
- 管理数据检查器等核心视图
- 统一的视图访问入口

## 编译和运行

确保已正确配置编译环境（使用 x64 Native Tools Command Prompt for VS 2022）：

```cmd
cd D:\develop\CPlusPlus\Dear_SDL\DearTsd
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

运行应用：
```cmd
build\bin\deartsdl_gui.exe
```

## 版本信息

- **版本**: 1.0.0
- **API 版本**: 1.0.0
- **作者**: DearTs Team
- **日期**: 2025

## 未来改进

可能的增强功能：

1. **拖拽排序**：允许用户拖拽视图项调整顺序
2. **收藏夹**：标记常用视图，快速访问
3. **视图分组**：自定义视图分组
4. **主题支持**：根据主题自动调整颜色
5. **快捷键**：为常用视图设置快捷键
6. **布局保存**：保存用户的视图布局配置
7. **最近使用**：显示最近打开的视图
8. **视图预览**：悬停显示视图缩略图

## 故障排除

### 侧边栏不显示

确保：
1. Navigation 插件已成功加载（检查日志）
2. SidebarView 已注册到 ContentRegistry
3. 视图的 `m_window_open` 状态为 true

### 视图状态不更新

确保：
1. 视图正确继承 `ViewWindow`
2. `get_window_open_state()` 方法正常工作
3. 插件加载顺序正确

### 搜索不工作

确保：
1. 搜索缓冲区正确初始化
2. `ImGui::IsItemEdited()` 检测到编辑事件
3. 字符串转换正常工作

## 许可证

遵循 DearTs 框架的许可证。

## 贡献

欢迎提交问题报告和改进建议！

---

**DearTs Framework** - 现代化的 C++20 应用开发框架
