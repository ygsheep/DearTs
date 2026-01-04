# TestPlugin - 插件自动发现测试

这是一个用于测试 DearTs Framework 插件自动发现功能的简单插件。

## 功能

- 自动从 `plugins/` 目录加载
- 注册测试命令 `testplugin.hello`
- 验证插件生命周期（load, enable, disable, unload）
- 完整的日志记录

## 构建

### 方法 1: 作为独立项目构建

```bash
cd plugins/test_plugin
cmake -B build -S .
cmake --build build --config Release
```

### 方法 2: 集成到主项目

在主项目的 `CMakeLists.txt` 中添加：

```cmake
# 在主 CMakeLists.txt 末尾添加
add_subdirectory(plugins/test_plugin)
```

然后正常构建主项目：

```bash
cmake -B build -S .
cmake --build build --config Release
```

## 输出

构建后，插件 DLL 会输出到：
- Windows: `build/plugins/test_plugin.dll`
- Linux: `build/plugins/libtest_plugin.so`
- macOS: `build/plugins/libtest_plugin.dylib`

## 使用

1. **构建插件**
   ```bash
   # 在项目根目录
   cmake --build build --config Release
   ```

2. **运行应用**
   ```bash
   # Windows
   ./build/bin/Debug/DearTsApp.exe

   # Linux/macOS
   ./build/bin/DearTsApp
   ```

3. **查看日志**
   应用启动时会自动扫描 `plugins/` 目录并加载插件。

   预期日志输出：
   ```
   [INFO] Loading builtin plugins...
   [INFO] Auto-discovering plugins in: plugins
   [INFO] Scanning directory for plugins: plugins
   [INFO] Loading plugin from: plugins/test_plugin.dll
   [INFO] Loaded library: plugins/test_plugin.dll
   [INFO] TestPlugin: dearts_create_plugin() called
   [INFO] Loading plugin: TestPlugin
   [INFO] TestPlugin: on_load() called
   [INFO] TestPlugin: Registered test command 'testplugin.hello'
   [INFO] Plugin loaded successfully: TestPlugin
   [INFO] Enabling plugin: TestPlugin
   [INFO] TestPlugin: on_enable() called - Plugin is now active
   [INFO] Successfully loaded plugin: TestPlugin from test_plugin.dll
   [INFO] Auto-discovered 1 external plugins
   [INFO] Plugin system initialized: 8 total plugins
   ```

4. **测试命令**
   在应用中打开命令面板（Ctrl+Shift+P），输入 `testplugin.hello` 执行。

   预期日志输出：
   ```
   [INFO] Hello from TestPlugin!
   ```

## 配置

可以通过配置文件控制插件自动发现行为：

```json
{
  "plugins": {
    "directory": "plugins",
    "auto_load": true,
    "enabled": ["TestPlugin"],
    "disabled": []
  }
}
```

- `plugins.directory`: 插件扫描目录（默认：`plugins`）
- `plugins.auto_load`: 是否启用自动发现（默认：`true`）
- `plugins.enabled`: 启用的插件列表（空列表表示全部启用）
- `plugins.disabled`: 禁用的插件列表

## 运行时重新扫描

在命令面板中执行：
```
plugins.rescan
```

这会重新扫描插件目录并加载新插件。

## 插件导出函数

此插件实现了两个必需的导出函数：

```cpp
extern "C" {
    __declspec(dllexport) IPlugin* dearts_create_plugin();
    __declspec(dllexport) void dearts_destroy_plugin(IPlugin* plugin);
}
```

**重要**：
- 函数名必须是 `dearts_create_plugin` 和 `dearts_destroy_plugin`
- 必须使用 `extern "C"` 避免名称修饰（name mangling）
- 必须使用 `__declspec(dllexport)` 导出符号（Windows）

## 故障排除

### 插件未加载

1. **检查日志**：查看是否有错误信息
2. **验证文件位置**：确保 DLL 在 `plugins/` 目录中
3. **检查符号导出**：
   - Windows: 使用 Dependency Walker 或 `dumpbin /EXPORTS`
   - Linux: 使用 `nm -D` 或 `objdump -T`
   - macOS: 使用 `nm -g`

### 符号未找到错误

确保：
- 使用了 `extern "C"` 包裹导出函数
- 函数名拼写正确（`dearts_create_plugin`, `dearts_destroy_plugin`）
- 使用了正确的导出宏

### API 版本不匹配

检查插件返回的 `api_version` 是否与框架版本（`1.0.0`）匹配。

## 扩展

你可以基于此模板创建自己的插件：

1. 复制 `plugins/test_plugin/` 目录
2. 修改类名和插件信息
3. 实现 `IPlugin` 接口
4. 更新 CMakeLists.txt 中的项目名称
5. 构建并测试

## 参考文档

- 插件系统 API: `dearts-dev/references/plugin_system_api.md`
- 插件开发指南: `docs/plugin_system_guide.md`
- 插件快速开始: `plugins/QUICKSTART.md`
