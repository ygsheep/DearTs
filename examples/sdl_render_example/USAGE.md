# SDL3 + ImGui 混合渲染示例 - 快速开始

## 快速运行

### 方式 1：使用启动脚本（推荐）

双击运行 `run.bat`

### 方式 2：命令行运行

```bash
cd build/bin/Debug
./sdl_render_example.exe
```

### 方式 3：在 CLion 中运行

1. 打开 `examples/sdl_render_example/main.cpp`
2. 点击编辑器右上角的绿色运行按钮
3. 或按 `Shift + F10`

## 你将看到什么

程序运行后，你会看到一个窗口，包含：

1. **左侧主面板** - "SDL3 混合渲染示例" 窗口
   - 可收起的 "SDL3 渲染区域"
   - 在区域内看到实时渲染的动画内容

2. **动画内容包括**：
   - 🎨 渐变背景（深蓝 → 深紫）
   - 📏 网格线（50x50 像素）
   - 🔴 旋转的红色矩形
   - 🟢 脉冲的绿色圆圈
   - 🔵 反向旋转的蓝色圆圈
   - ✨ 黄色中心十字

## 交互操作

| 操作 | 效果 |
|------|------|
| **拖拽鼠标** | 平移视图（上下左右移动内容） |
| **滚动滚轮** | 缩放视图（放大/缩小 0.1x - 10x） |
| **点击标题栏** | 收起/展开渲染区域 |
| **点击"重置视图"按钮** | 恢复默认视图（居中、1x 缩放） |

## 技术亮点

### 1. SDL3 离屏渲染

```cpp
// 创建渲染目标纹理
SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, width, height);

// 渲染到纹理
SDL_SetRenderTarget(renderer, texture);
// ... SDL3 绘制命令 ...
```

### 2. ImGui 显示纹理

```cpp
// 转换为 ImTextureID
ImTextureID texture_id = (ImTextureID)texture;

// 在 ImGui 中显示
ImGui::Image(texture_id, size);
```

### 3. 视图变换

```cpp
// 应用缩放和平移
float scaled_width = display_width * scale;
ImVec2 p0(
    center_x - scaled_width / 2 + offset_x,
    center_y - scaled_height / 2 + offset_y
);
```

### 4. 鼠标交互

```cpp
// 检测鼠标悬停
if (ImGui::IsItemHovered()) {
    // 处理滚轮缩放
    float wheel = ImGui::GetIO().MouseWheel;
    scale *= (wheel > 0) ? 1.1f : 0.9f;
}

// 检测拖拽
if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetMouseDragDelta();
    offset_x += delta.x;
    offset_y += delta.y;
}
```

## 应用场景

这个技术可以用于：

- **游戏小地图** - 在小窗口显示游戏世界
- **数据可视化** - 显示大量实时数据图表
- **图像编辑器** - 预览和处理图像
- **调试工具** - 显示碰撞盒、路径等调试信息
- **性能监控** - 实时显示性能图表

## 下一步

尝试修改代码：

1. **改变动画速度** - 修改 `draw_sample_graphics()` 中的 `time` 参数系数
2. **添加新的图形** - 使用 SDL3 绘制函数添加更多图形
3. **调整颜色** - 修改 `SDL_SetRenderDrawColor()` 的 RGB 值
4. **改变纹理尺寸** - 修改 `TEXTURE_WIDTH` 和 `TEXTURE_HEIGHT` 常量
5. **添加更多交互** - 实现点击选择、右键菜单等

## 代码结构

```
main.cpp (约 600 行)
├── init()              - 初始化 SDL、ImGui、纹理
├── shutdown()          - 清理资源
├── render_to_texture() - SDL3 渲染到纹理
├── draw_sample_graphics() - 绘制示例图形
├── draw_gui()          - ImGui 界面
└── handle_events()     - SDL 事件处理
```

## 性能

- **更新频率**：30 FPS（每 33ms 更新一次纹理）
- **纹理尺寸**：800x600 像素
- **文件大小**：约 8.2 MB（静态链接）

## 故障排除

### 问题：程序无法启动

**解决**：确保 `build/bin/Debug/` 目录下有 `sdl_render_example.exe`

### 问题：看不到动画

**解决**：
1. 确保 "SDL3 渲染区域" 是展开状态
2. 检查控制台是否有错误信息

### 问题：性能卡顿

**解决**：
1. 降低更新频率（修改 `0.033f` 为 `0.1f`）
2. 减小纹理尺寸

## 参考资料

- [完整技术文档](README.md)
- [DearTs Framework](../../dearts-dev/SKILL.md)
- [SDL3 文档](https://wiki.libsdl.org/SDL3/)
- [ImGui 文档](https://github.com/ocornut/imgui)

---

**提示**：这是一个教学示例，展示了 SDL3 和 ImGui 混合渲染的基本技术。在实际项目中，你可能需要根据具体需求进行调整和优化。
