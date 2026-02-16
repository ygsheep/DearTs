#!/usr/bin/env bash
# DearTs 运行脚本 - 用于 NixOS 环境
# 此脚本确保所有必要的库路径都正确设置

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"
BIN_DIR="$BUILD_DIR/bin"

# 检查构建目录
if [ ! -d "$BIN_DIR" ]; then
    echo "错误：构建目录不存在: $BIN_DIR"
    echo "请先运行: cmake -B build && cmake --build build"
    exit 1
fi

# 使用 nix-shell 运行程序，自动设置所有库路径
cd "$SCRIPT_DIR"
echo "正在启动 DearTs 工具箱..."
echo ""
echo "提示："
echo "  - 按 Ctrl+Q 退出应用"
echo "  - 按 F11 切换全屏"
echo "  - 按 Ctrl+P 打开命令面板"
echo ""

# 检测会话类型，优先使用合适的后端
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "检测到 Wayland 会话，使用 Wayland 后端"
    export SDL_VIDEODRIVER=wayland
elif [ -n "$DISPLAY" ]; then
    echo "检测到 X11 会话，使用 X11 后端"
    export SDL_VIDEODRIVER=x11
else
    echo "未检测到显示服务器，尝试 Wayland"
    export SDL_VIDEODRIVER=wayland
fi

echo "使用后端: $SDL_VIDEODRIVER"
echo ""

# 在 nix-shell 中运行
nix-shell --run "cd \"$BIN_DIR\" && ./ChatManager"
