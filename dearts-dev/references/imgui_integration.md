# ImGui 集成指南

ImGui 已在 ImGuiLayer 中自动初始化。

## 基本使用

```cpp
ImGui::Begin("Window");
ImGui::Text("Hello, ImGui!");
if (ImGui::Button("Click me")) {
    // 处理点击
}
ImGui::End();
```

## Docking

```cpp
ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
```

更多内容请参考项目文档。
