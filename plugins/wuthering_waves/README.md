# 鸣潮抽卡记录插件

DearTs Framework 的鸣潮抽卡记录获取插件，自动从《鸣潮》游戏日志中提取抽卡记录 URL。

## 功能特性

- ✅ **自动查找游戏路径** - 支持多种方式查找游戏安装位置
- ✅ **从日志提取 URL** - 自动解析游戏日志文件，提取抽卡记录链接
- ✅ **一键复制** - 提取的 URL 自动复制到剪贴板
- ✅ **手动指定路径** - 支持用户手动指定游戏安装目录
- ✅ **DLL 插件** - 动态加载，无需重新编译主程序

## 使用方法

### 1. 自动加载

插件 DLL 会自动从 `bin/plugins/` 目录加载，无需手动配置。

### 2. 打开抽卡记录界面

1. 启动 DearTs 应用
2. 在游戏中打开**抽卡记录**界面
3. 返回 DearTs，点击菜单或使用命令面板打开插件功能

### 3. 提取 URL

1. 点击 **"扫描游戏路径"** 按钮
2. 插件会自动扫描游戏安装位置并查找抽卡记录 URL
3. 找到后，URL 会自动复制到剪贴板

### 4. 导入抽卡记录

1. 打开抽卡记录分析网站：https://mc.appfeng.com/gachaLog
2. 点击 **"导入记录"** 按钮
3. 粘贴 URL 并确认

## 支持的扫描方式

| 扫描方式 | 说明 |
|---------|------|
| MUI 缓存 | 从 Windows 系统缓存中查找 |
| 防火墙 | 从防火墙规则中查找 |
| 注册表 | 从注册表卸载信息中查找 |
| 常见路径 | 扫描常见游戏安装位置 |

## 技术细节

### 插件信息

```cpp
{
    "name": "WutheringWaves",
    "author": "DearTs Team",
    "description": "鸣潮抽卡记录获取工具 - 自动提取抽卡记录 URL",
    "version": "1.0.0",
    "api_version": "1.0.0"
}
```

### 日志文件位置

插件会检查以下日志文件：

- `Client\Saved\Logs\Client.log`
- `Client\Binaries\Win64\ThirdParty\KrPcSdk_Global\KRSDKRes\KRSDKWebView\debug.log`

### URL 格式

```
https://aki-gm-resources(-oversea)?.aki-game.(net|com)/aki/gacha/index.html#/record*
```

## 构建说明

### 依赖

- DearTs Framework 1.0.0+
- CMake 3.20+
- C++20 编译器 (MSVC 2022 / GCC 11+ / Clang 13+)

### 构建命令

```bash
# 构建插件 DLL
cmake --build build --config Debug --target wuthering_waves

# 输出位置
build/bin/plugins/wuthering_waves.dll
```

## 常见问题

### Q: 扫描不到游戏路径？

A: 请尝试以下方法：
1. 确认游戏已正确安装
2. 尝试手动指定游戏安装目录
3. 检查是否已打开游戏中的抽卡记录界面

### Q: 找不到 URL？

A: 请确保：
1. 已在游戏中打开抽卡记录界面
2. 游戏版本与插件兼容
3. 日志文件存在且可读

### Q: DLL 加载失败？

A: 请检查：
1. DLL 文件位于 `bin/plugins/` 目录
2. 依赖的 SDL3.dll 等库文件存在
3. 插件 API 版本与主程序兼容

## 更新日志

### v1.0.0 (2025-02-19)

- ✨ 初始版本发布
- ✅ 支持自动游戏路径查找
- ✅ 支持从日志提取抽卡记录 URL
- ✅ 支持一键复制到剪贴板
- ✅ DLL 动态加载支持

## 许可证

MIT License

## 联系方式

- 项目地址：[DearTs Framework](https://github.com/DearTsTeam/DearTs)
- 问题反馈：请提交 Issue

---

**注意**：此插件仅用于提取游戏生成的抽卡记录 URL，不涉及任何游戏修改或作弊行为。
