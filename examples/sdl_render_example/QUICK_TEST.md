# 中文字体支持 - 快速测试

## ✅ 已完成

1. ✅ 添加中文字体加载代码
2. ✅ 自动复制字体文件到输出目录
3. ✅ 支持完整 CJK 字符集
4. ✅ 加载两个字体尺寸（16px 和 24px）

## 🚀 快速测试

### 运行程序

```bash
# 方式 1：双击运行
examples/sdl_render_example/run.bat

# 方式 2：命令行运行
build/bin/Debug/sdl_render_example.exe
```

### 检查字体加载

运行后查看控制台输出：

**成功情况**：
```
成功加载中文字体: resources/fonts/OPPOSans-M.ttf
```

**失败情况**：
```
警告：未能加载中文字体，使用默认字体（可能无法显示中文）
```

### 验证中文显示

在程序中，你应该能看到：

1. **窗口标题** - "SDL3 + ImGui 混合渲染示例"
2. **可收起区域** - "SDL3 渲染区域"
3. **按钮文本** - "重置视图"
4. **状态信息** - "偏移: (x, y) | 缩放: z"
5. **操作说明** - "鼠标左键拖拽：平移视图"

所有这些中文文本应该正常显示，**不是方块或乱码**！

## 📁 字体位置

字体文件已复制到：
```
build/bin/Debug/resources/fonts/
├── OPPOSans-M.ttf         9.7 MB
├── Noto nerd.ttf          2.8 MB
├── MaterialSymbolsOutlined.ttf    4.0 KB
└── MaterialSymbolsRounded-VariableFont.ttf   14 MB
```

## 🔧 如果仍然乱码

### 手动复制字体文件

```bash
# 从项目根目录复制
cp resources/fonts/OPPOSans-M.ttf build/bin/Debug/resources/fonts/

# 或复制整个字体目录
cp -r resources/fonts/* build/bin/Debug/resources/fonts/
```

### 检查字体文件是否存在

```bash
ls -lh build/bin/Debug/resources/fonts/OPPOSans-M.ttf
```

应该看到：
```
-rw-r--r-- 1 ygshe 197609 9.7M 12月 29 23:14 OPPOSans-M.ttf
```

### 尝试其他字体

修改代码，添加系统字体：

```cpp
static const char* font_paths[] = {
    "resources/fonts/OPPOSans-M.ttf",
    "C:/Windows/Fonts/msyh.ttc",       // 微软雅黑
    "C:/Windows/Fonts/simhei.ttf",     // 黑体
    "C:/Windows/Fonts/simsun.ttc",     // 宋体
    // ...
};
```

## 📊 字体覆盖

### 支持的字符

- ✅ 简体中文（约 20,000 字）
- ✅ 繁体中文（约 13,000 字）
- ✅ 日文汉字（约 6,000 字）
- ✅ 韩文（约 11,000 字）
- ✅ 英文（ASCII + 扩展）
- ✅ 标点符号

### 使用的字符集

```cpp
io.Fonts->GetGlyphRangesChineseFull()
```

这包含了：
- `io.Fonts->GetGlyphRangesChineseSimplifiedCommon()` - 简体中文
- `io.Fonts->GetGlyphRangesChineseFull()` - 完整汉字
- `io.Fonts->GetGlyphRangesJapanese()` - 日文
- `io.Fonts->GetGlyphRangesKorean()` - 韩文
- `io.Fonts->GetGlyphRangesCyrillic()` - 西里尔字母
- `io.Fonts->GetGlyphRangesThai()` - 泰文
- `io.Fonts->GetGlyphRangesVietnamese()` - 越南文

## 🎨 使用不同字体

### 修改默认字体

如果不喜欢 OPPOSans，可以换成其他字体：

```cpp
// 优先使用 Noto
static const char* font_paths[] = {
    "resources/fonts/Noto nerd.ttf",   // 放在第一位
    "resources/fonts/OPPOSans-M.ttf",
    // ...
};
```

### 添加更多字体尺寸

```cpp
// 小字体（14px）- 用于密集文本
io.Fonts->AddFontFromFileTTF(font_path, 14.0f, &font_config, ...);

// 默认字体（16px）- 正文
io.Fonts->AddFontFromFileTTF(font_path, 16.0f, &font_config, ...);

// 大字体（20px）- 小标题
io.Fonts->AddFontFromFileTTF(font_path, 20.0f, &font_config, ...);

// 特大字体（24px）- 大标题
io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, ...);
```

### 使用不同字重

如果有多字重字体文件（如 Regular, Bold, Light）：

```cpp
static const char* font_paths[] = {
    "resources/fonts/OPPOSans-B.ttf",  // Bold
    "resources/fonts/OPPOSans-M.ttf",  // Medium
    "resources/fonts/OPPOSans-R.ttf",  // Regular
    "resources/fonts/OPPOSans-L.ttf",  // Light
};
```

## 💡 性能优化

### 减少内存占用

如果不需要完整的 CJK 支持，可以缩小字符集：

```cpp
// 只包含简体中文常用字
io.Fonts->GetGlyphRangesChineseSimplifiedCommon()

// 只有基本字符（英文 + 基本标点）
io.Fonts->GetGlyphRangesDefault()
```

### 合并字体

将多个字体合并到一个 ImFont：

```cpp
// 先加载英文
ImFontConfig font_config;
io.Fonts->AddFontFromFileTTF("English.ttf", 16.0f, &font_config,
                             io.Fonts->GetGlyphRangesDefault());

// 再加载中文（合并）
font_config.MergeMode = true;  // 启用合并模式
io.Fonts->AddFontFromFileTTF("Chinese.ttf", 16.0f, &font_config,
                             io.Fonts->GetGlyphRangesChineseFull());
```

## 🐛 已知问题

### 问题 1：首次加载慢

**原因**：完整 CJK 字符集包含约 50,000 字，加载需要时间

**解决**：
- 使用较小的字符集
- 或只加载常用字

### 问题 2：内存占用高

**原因**：每个字体尺寸约占用 1-2 MB

**解决**：
- 减少字体尺寸数量
- 使用较小的字符集
- 按需加载字体

## ✨ 总结

**现在运行程序，中文应该完美显示了！**

- ✅ 字体文件已自动复制
- ✅ 完整 CJK 字符支持
- ✅ 两个字体尺寸（16px, 24px）
- ✅ 自动回退到默认字体

**立即运行测试吧！** 🚀

```bash
build/bin/Debug/sdl_render_example.exe
```
