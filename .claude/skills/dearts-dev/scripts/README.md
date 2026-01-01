# Scripts

This directory contains utility scripts for DearTs Framework development.

## 🚀 Build Scripts

### `build.py` (推荐)
Python 构建脚本，支持多平台和多种构建类型。

**特性:**
- 支持 Debug/Release/RelWithDebInfo 模式
- 自动依赖检查
- 并行构建支持
- 彩色输出
- 错误处理

**使用:**
```bash
# Release 构建
python scripts/build.py --config Release

# Debug 构建
python scripts/build.py --config Debug

# 清理后构建
python scripts/build.py --clean --config Release

# 显示帮助
python scripts/build.py --help
```

**选项:**
- `--config {Debug,Release,RelWithDebInfo}` - 构建配置
- `--clean` - 构建前清理
- `--verbose` - 详细输出
- `--parallel N` - 并行任务数

---

### `build_debug.bat`
Windows Debug 快速构建脚本。

**使用:**
```cmd
scripts\build_debug.bat
```

---

### `build_temp.bat`
Windows 临时构建脚本。

**使用:**
```cmd
scripts\build_temp.bat
```

---

## 📋 手动构建（无脚本）

如果脚本不可用，可以手动使用 CMake:

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Clean
cmake --build build --target clean
```

---

## 💡 提示

- 推荐使用 `build.py` - 功能最全面
- Windows 可使用 `.bat` 快速脚本
- 构建输出在 `build/` 目录
- 可执行文件在 `build/bin/`

---

**Python 版本要求**: 3.7+
**CMake 版本要求**: 3.20+
