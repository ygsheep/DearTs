@echo off
REM SDL3 + ImGui 混合渲染示例启动脚本

echo ====================================
echo SDL3 + ImGui Hybrid Render Example
echo ====================================
echo.
echo 操作说明：
echo   - 鼠标左键拖拽：平移视图
echo   - 鼠标滚轮：缩放视图
echo   - 点击标题栏：收起/展开区域
echo.
echo 正在启动...
echo.

start "" "%~dp0..\..\build\bin\Debug\sdl_render_example.exe"

pause
