# Examples

This directory contains DearTs Framework examples and tutorials.

## 📚 示例列表

### SDL3 + ImGui 混合渲染

**文件:**
- `SDL3_ImGui_Hybrid_Rendering.md` - SDL3 + ImGui 混合渲染指南 (7.7 KB)

**描述:**
展示如何在 DearTs Framework 中同时使用 SDL3 渲染和 ImGui UI。

**内容包括:**
- SDL3 渲染上下文创建
- ImGui 与 SDL3 集成
- 混合渲染管线
- 帧管理
- 资源管理

**适用场景:**
- 需要高性能渲染的游戏
- 复杂的图形可视化
- 自定义渲染需求

---

### 交互式 SDL 渲染

**文件:**
- `interactive_sdl_render.md` - 交互式 SDL 渲染完整指南 (17.5 KB)
- `sdl_render_view.hpp` - SDL 渲染视图头文件
- `sdl_render_view.cpp` - SDL 渲染视图实现

**描述:**
完整的交互式 SDL 渲染示例，展示如何在 DearTs 中创建自定义 SDL 渲染视图。

**特性:**
- SDL2 纹理渲染
- 鼠标交互处理
- 实时动画
- 与 ImGui UI 集成
- 完整的事件处理

**代码示例:**
```cpp
#include "sdl_render_view.hpp"

// 注册视图
ContentRegistry::Views::add<SdlRenderView>();
```

**适用场景:**
- 需要直接操作 SDL 的应用
- 图像处理工具
- 数据可视化
- 游戏编辑器

---

## 🚀 快速开始

### 运行混合渲染示例

1. 阅读指南:
   ```bash
   # 阅读混合渲染指南
   cat examples/SDL3_ImGui_Hybrid_Rendering.md
   ```

2. 查看代码示例:
   ```cpp
   // 参考指南中的代码
   ```

### 运行交互式渲染示例

1. 阅读完整指南:
   ```bash
   cat examples/interactive_sdl_render.md
   ```

2. 复制源文件:
   ```bash
   cp examples/sdl_render_view.hpp your_project/
   cp examples/sdl_render_view.cpp your_project/
   ```

3. 在你的应用中注册:
   ```cpp
   #include "sdl_render_view.hpp"

   class MyApp : public Application {
       bool on_initialize() override {
           ContentRegistry::Views::add<SdlRenderView>();
           return true;
       }
   };
   ```

---

## 📖 学习路径

**初学者:**
1. 阅读 `SDL3_ImGui_Hybrid_Rendering.md` - 了解基础概念
2. 运行项目查看内置视图
3. 修改示例代码进行实验

**进阶:**
1. 阅读 `interactive_sdl_render.md` - 学习完整实现
2. 研究 `sdl_render_view.cpp` - 理解细节
3. 创建自己的渲染视图

**高级:**
1. 参考示例创建自定义渲染器
2. 优化渲染性能
3. 集成复杂图形库（OpenGL, Vulkan）

---

## 💡 提示

- 示例代码是完整可运行的
- 所有示例都遵循框架最佳实践
- 可以直接复制代码到你的项目
- 参考注释了解实现细节

---

## 🔗 相关资源

- **API 文档**: `../references/`
- **代码模板**: `../assets/`
- **插件示例**: `../../plugins/builtin/`
- **ImGui 文档**: https://github.com/ocornut/imgui
- **SDL 文档**: https://wiki.libsdl.org/

---

**最后更新**: 2025-12-30
