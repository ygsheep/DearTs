#pragma once

#include "layout_base.h"
#include <string>
#include <imgui.h>
#include <memory>

// 添加资源管理器相关头文件
#include "../../resource/resource_manager.h"
#include "../../resource/font_resource.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 标题栏布局类
 * 实现自定义标题栏功能，包括拖拽、最小化、最大化、关闭等
 */
class TitleBarLayout : public LayoutBase {
public:
    /**
     * @brief 构造函数
     */
    explicit TitleBarLayout();
    
    /**
     * @brief 渲染标题栏布局
     */
    void render() override;
    
    /**
     * @brief 更新标题栏布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    void updateLayout(float width, float height) override;
    
    /**
     * @brief 处理标题栏事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;
    
    /**
     * @brief 设置窗口标题
     */
    void setWindowTitle(const std::string& title);
    
    /**
     * @brief 获取窗口标题
     */
    const std::string& getWindowTitle() const { return windowTitle_; }
    
    /**
     * @brief 设置是否已最大化
     */
    void setMaximized(bool maximized) { isMaximized_ = maximized; }
    
    /**
     * @brief 检查是否已最大化
     */
    bool isMaximized() const { return isMaximized_; }
    
    /**
     * @brief 保存窗口正常状态位置和大小
     */
    void saveNormalState(int x, int y, int width, int height);
    
    /**
     * @brief 获取保存的正常状态X坐标
     */
    int getNormalX() const { return normalX_; }
    
    /**
     * @brief 获取保存的正常状态Y坐标
     */
    int getNormalY() const { return normalY_; }
    
    /**
     * @brief 获取保存的正常状态宽度
     */
    int getNormalWidth() const { return normalWidth_; }
    
    /**
     * @brief 获取保存的正常状态高度
     */
    int getNormalHeight() const { return normalHeight_; }
    
    /**
     * @brief 获取标题栏高度
     */
    float getTitleBarHeight() const { return titleBarHeight_; }
    
    /**
     * @brief 检查搜索对话框是否显示
     */
    bool isSearchDialogVisible() const { return showSearchDialog_; }
    
    /**
     * @brief 显示/隐藏搜索对话框
     */
    void setShowSearchDialog(bool show) { showSearchDialog_ = show; }
    
    /**
     * @brief 获取搜索缓冲区
     */
    char* getSearchBuffer() { return searchBuffer_; }
    
    /**
     * @brief 设置搜索输入框焦点状态
     */
    void setSearchInputFocused(bool focused) { searchInputFocused_ = focused; }

    /**
     * @brief 更新窗口状态（与SDL同步）
     */
    void updateWindowState();

    /**
     * @brief 获取窗口的实际状态（查询SDL）
     */
    bool isActuallyMaximized() const;

private:
    // 标题栏相关属性
    std::string windowTitle_;           ///< 窗口标题
    bool isDragging_;                   ///< 是否正在拖拽
    bool isMaximized_;                  ///< 是否已最大化
    int dragOffsetX_, dragOffsetY_;     ///< 拖拽偏移量
    float titleBarHeight_;              ///< 标题栏高度
    
    // 搜索功能相关
    bool showSearchDialog_;             ///< 是否显示搜索对话框
    char searchBuffer_[256];            ///< 搜索输入缓冲区
    bool searchInputFocused_;           ///< 搜索输入框是否获得焦点
    
    // 窗口状态保存
    int normalX_, normalY_;             ///< 正常状态位置
    int normalWidth_, normalHeight_;    ///< 正常状态大小
    
#if defined(_WIN32)
    HWND hwnd_;                         ///< Windows窗口句柄
#endif
    
    // 图片资源相关
    std::shared_ptr<DearTs::Core::Resource::TextureResource> iconTexture_;  ///< 标题栏图标纹理

    // 事件处理相关
    bool buttonClicked_;                  ///< 按钮是否被点击（防止SDL事件干扰）
    
    /**
     * @brief 渲染标题文本
     */
    void renderTitle();
    
    /**
     * @brief 渲染搜索框
     */
    void renderSearchBox();
    
    /**
     * @brief 渲染窗口控制按钮
     */
    void renderControlButtons();
    
    /**
     * @brief 渲染搜索对话框
     */
    void renderSearchDialog();
    
    /**
     * @brief 处理键盘快捷键
     */
    void handleKeyboardShortcuts();
    
    /**
     * @brief 检查是否在标题栏区域
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @return 是否在标题栏区域
     */
    bool isInTitleBarArea(int x, int y) const;
    
    /**
     * @brief 开始拖拽窗口
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void startDragging(int mouseX, int mouseY);
    
    /**
     * @brief 更新拖拽状态
     * @param mouseX 当前鼠标X坐标
     * @param mouseY 当前鼠标Y坐标
     */
    void updateDragging(int mouseX, int mouseY);
    
    /**
     * @brief 停止拖拽
     */
    void stopDragging();
    
    /**
     * @brief 最小化窗口
     */
    void minimizeWindow();
    
    /**
     * @brief 最大化/还原窗口
     */
    void toggleMaximize();
    
    /**
     * @brief 关闭窗口
     */
    void closeWindow();
};

} // namespace Window
} // namespace Core
} // namespace DearTs