@echo off
REM SDL3 + ImGui 对象级交互示例启动脚本

echo ========================================
echo SDL3 + ImGui Object Interaction
echo ========================================
echo.
echo 功能演示：
echo   - 鼠标移动：悬停高亮对象（黄色）
echo   - 左键点击：选择对象（青色）
echo   - 右键拖拽：平移视图
echo   - 鼠标滚轮：缩放视图
echo   - ESC 键：取消选择
echo   - 空格键：暂停/恢复动画
echo.
echo 正在启动...
echo.

start "" "%~dp0..\..\build\bin\Debug\sdl_interactive_example.exe"

pause
