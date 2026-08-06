# Markdown 渲染示例程序

## 概述

这是一个展示如何使用 [imgui_markdown](https://github.com/juliettef/imgui_markdown) 库在 Dear ImGui 中渲染 Markdown 内容的独立示例程序。

## 功能特性

- ✅ **5种示例模式** - 基本语法、文档说明、中文支持、README 风格、实时编辑
- ✅ **完整 Markdown 支持** - 标题、强调、列表、链接、缩进、水平线
- ✅ **实时编辑预览** - 左侧编辑，右侧实时预览
- ✅ **中文字体支持** - 完美显示中文内容
- ✅ **自定义样式** - 自定义标题颜色和字体
- ✅ **跨平台链接** - 支持 Windows/macOS/Linux 打开浏览器

## 支持的 Markdown 语法

### 标题
```markdown
# H1 标题
## H2 标题
### H3 标题
```

### 强调
```markdown
*斜体* 或 _斜体_
**粗体** 或 __粗体__
```

### 列表
```markdown
  * 列表项 1
  * 列表项 2
    * 子项 2.1
    * 子项 2.2
```

### 缩进
```markdown
普通文本
  缩进一级
    缩进二级
```

### 链接
```markdown
[链接文字](https://example.com)
```

### 水平线
```markdown
***
___
```

## 示例模式

### 1. 基本语法
展示 Markdown 的基本语法，包括文本样式、列表、缩进等。

### 2. 文档说明
展示如何使用 Markdown 编写技术文档，包含各种语法的详细说明。

### 3. 中文支持
演示中文内容的渲染效果，验证中文字体的正确显示。

### 4. README 风格
模拟 GitHub README 的渲染效果，展示更复杂的排版。

### 5. 实时编辑
分屏布局，左侧编辑 Markdown，右侧实时预览渲染结果。

## 编译和运行

### 编译
```bash
# 在 build 目录中
cmake --build . --config Debug --target markdown_example
```

### 运行
```bash
# Windows
./build/examples/markdown_example/Debug/markdown_example.exe

# Linux/macOS
./build/examples/markdown_example/markdown_example
```

## 使用方法

1. **选择示例** - 使用顶部菜单或按钮选择不同的示例模式
2. **查看效果** - 观察不同 Markdown 语法在 ImGui 中的渲染效果
3. **实时编辑** - 在"实时编辑"模式下，左侧输入 Markdown，右侧实时预览
4. **点击链接** - 点击 Markdown 中的链接会在浏览器中打开

## 代码示例

### 基本使用

```cpp
#include "imgui_markdown.h"

// 链接回调
void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    // 打开链接...
}

// 图片回调
ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_) {
    // 返回图片数据...
}

// 渲染 Markdown
void RenderMarkdown(const std::string& markdown_) {
    ImGui::MarkdownConfig mdConfig;
    mdConfig.linkCallback = LinkCallback;
    mdConfig.imageCallback = ImageCallback;
    mdConfig.headingFormats[0] = { fontH1, true };
    mdConfig.headingFormats[1] = { fontH2, true };
    mdConfig.headingFormats[2] = { fontH3, false };

    ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}
```

### 自定义样式

```cpp
void MarkdownFormatCallback(const ImGui::MarkdownFormatInfo& info, bool start_) {
    ImGui::defaultMarkdownFormatCallback(info, start_);

    // 自定义 H2 标题颜色
    if (info.type == ImGui::MarkdownFormatType::HEADING && info.level == 2) {
        if (start_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        } else {
            ImGui::PopStyleColor();
        }
    }
}
```

### 在 ImGui 窗口中使用

```cpp
void render_ui() {
    ImGui::Begin("Markdown Viewer");

    const std::string markdown = u8R"(
        # 标题
        这是 **粗体** 和 *斜体* 文本。
    )";

    RenderMarkdown(markdown);

    ImGui::End();
}
```

## 配置说明

### MarkdownConfig 结构

```cpp
struct MarkdownConfig {
    MarkdownLinkCallback*   linkCallback = nullptr;      // 链接点击回调
    MarkdownTooltipCallback* tooltipCallback = nullptr;  // 工具提示回调
    MarkdownImageCallback*  imageCallback = nullptr;     // 图片加载回调
    const char*             linkIcon = "";               // 链接图标
    MarkdownHeadingFormat   headingFormats[3];          // 标题格式
    void*                   userData = nullptr;          // 用户数据
    MarkdownFormalCallback* formatCallback = nullptr;    // 格式回调
};
```

### MarkdownHeadingFormat 结构

```cpp
struct MarkdownHeadingFormat {
    ImFont* font;          // 标题字体
    bool    separator;     // 是否显示分隔线
    float   fontSize;      // 字体大小（动态字体）
};
```

## 不支持的语法

以下语法组合当前不被支持：

- 标题中包含链接、图片或强调
- 强调中包含链接或图片
- 多行强调（换行会打断强调）

## 学习要点

1. **字体配置** - 为不同级别的标题配置不同的字体
2. **回调函数** - 实现链接、图片、样式的自定义处理
3. **字符串类型** - 使用 `u8R"(...)"` 原始字符串字面量支持中文
4. **内存管理** - MarkdownConfig 可以在每次渲染时重新配置

## 应用场景

- **帮助文档** - 在应用内显示帮助文档
- **README 显示** - 显示项目的 README 文件
- **注释说明** - 显示带格式的注释和说明
- **聊天消息** - 渲染带格式的聊天消息
- **日志查看** - 显示格式化的日志内容

## 扩展建议

尝试修改示例代码：
- 添加代码块支持（使用不同的字体或背景色）
- 实现图片加载功能（从文件加载真实图片）
- 添加表格支持
- 实现主题切换（亮色/暗色）
- 添加搜索功能

## 相关资源

- [imgui_markdown GitHub](https://github.com/juliettef/imgui_markdown)
- [Dear ImGui GitHub](https://github.com/ocornut/imgui)
- [Markdown 规范](https://commonmark.org/)

## 许可证

imgui_markdown 使用 zlib 许可证，详见 `third_party/imgui_markdown/imgui_markdown.h`
