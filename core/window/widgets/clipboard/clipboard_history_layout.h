#pragma once

#include "../../layouts/layout_base.h"
#include "clipboard_manager.h"
#include "text_segmentation_window.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <imgui.h>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 剪切板历史布局类
 *
 * 显示剪切板历史记录的布局组件，具有以下特性：
 * - 透明背景效果
 * - 历史记录列表显示
 * - 搜索和过滤功能
 * - 双击打开分词窗口
 * - 收藏和分类管理
 * - 快捷键支持
 */
class ClipboardHistoryLayout : public LayoutBase {
public:
    ClipboardHistoryLayout();
    ~ClipboardHistoryLayout() override;

    // LayoutBase 接口
    void render() override;
    void updateLayout(float width, float height) override;
    void handleEvent(const SDL_Event& event) override;
    void renderInFixedArea(float contentX, float contentY, float contentWidth, float contentHeight) override;

    // 窗口显示/隐藏
    void showWindow();
    void hideWindow();
    bool isVisible() const { return is_visible_; }
    void toggleWindow();

    // 数据操作
    void refreshHistory();
    void clearHistory();
    void setSelectedItem(const std::string& id);

    // 启动剪切板监听
    void startClipboardMonitoring(SDL_Window* sdl_window);

private:
    // 渲染组件
    void renderTranslucentBackground();
    void renderHeader();                   // 头部工具栏
    void renderSearchBox();                // 搜索框
    void renderFilterBar();                // 过滤栏
    void renderHistoryList();              // 历史记录列表
    void renderFooter();                   // 底部状态栏

    // 历史记录项渲染
    void renderHistoryItem(const ClipboardItem& item, int index);
    void renderItemContent(const ClipboardItem& item);
    void renderItemUrls(const ClipboardItem& item);
    void renderItemActions(const ClipboardItem& item, int index);

    // 交互处理
    void handleMouseInteraction();
    void handleKeyboardInput();
    void handleSearchInput();
    void handleFilterSelection();
    void handleItemDoubleClick();
    void handleContextMenu();

    // 数据管理
    void updateFilteredList();
    void searchItems(const std::string& keyword);
    void filterByCategory(const std::string& category);
    void toggleFavorites();

    // UI状态管理
    void updateColors();
    void resetInteractionState();

    // 功能操作
    void copySelectedItem();
    void deleteSelectedItem();
    void toggleFavoriteItem();
    void openSegmentationWindow(const ClipboardItem& item);
    void toggleSegmentationWindow();  // 切换分词助手窗口显示/隐藏
    void exportHistory();
    void importHistory();

    // 剪切板监听回调
    void onClipboardContentChanged(const std::string& content);

    // 布局计算
    void calculateLayout();
    ImVec2 calculateItemSize(const ClipboardItem& item);
    void arrangeItems();

    // 快捷键处理
    void handleShortcuts();

    // 成员变量
    std::unique_ptr<ClipboardManager> clipboard_manager_;   // 剪切板管理器
    std::unique_ptr<TextSegmentationWindow> segmentation_window_; // 分词窗口

    // 数据
    std::vector<ClipboardItem> history_items_;            // 原始历史记录
    std::vector<ClipboardItem> filtered_items_;           // 过滤后的记录
    std::vector<std::string> categories_;                 // 分类列表

    // 状态
    bool is_visible_;                                    // 窗口可见性
    int selected_item_index_;                            // 选中项索引
    int hovered_item_index_;                             // 悬停项索引
    std::string selected_item_id_;                       // 选中项ID
    bool show_favorites_only_;                           // 只显示收藏
    std::string current_filter_;                         // 当前过滤器

    // 搜索相关
    char search_buffer_[256];                            // 搜索缓冲区
    bool search_focused_;                                // 搜索框焦点
    std::string last_search_keyword_;                    // 上次搜索关键词

    // UI配置
    float window_opacity_;                               // 窗口透明度
    ImVec2 window_size_;                                 // 窗口大小
    ImVec2 window_position_;                             // 窗口位置
    ImVec2 content_margin_;                              // 内容边距

    // 颜色配置
    struct Colors {
        ImVec4 window_bg;              // 窗口背景
        ImVec4 header_bg;              // 头部背景
        ImVec4 item_normal;            // 普通项目背景
        ImVec4 item_hovered;           // 悬停项目背景
        ImVec4 item_selected;          // 选中项目背景
        ImVec4 item_favorite;          // 收藏项目背景
        ImVec4 text_normal;            // 普通文本
        ImVec4 text_dimmed;            // 暗淡文本
        ImVec4 text_url;               // URL文本
        ImVec4 border_normal;          // 普通边框
        ImVec4 border_hovered;         // 悬停边框
        ImVec4 border_selected;        // 选中边框
        ImVec4 search_bg;              // 搜索框背景
        ImVec4 button_normal;          // 普通按钮
        ImVec4 button_hovered;         // 悬停按钮
    } colors_;

    // 布局参数
    struct Layout {
        float header_height = 50.0f;     // 头部高度
        float search_height = 40.0f;     // 搜索框高度
        float filter_height = 35.0f;     // 过滤栏高度
        float footer_height = 30.0f;     // 底部高度
        float item_min_height = 60.0f;   // 项目最小高度
        float item_padding = 8.0f;       // 项目内边距
        float item_spacing = 2.0f;       // 项目间距
        float corner_radius = 4.0f;      // 圆角半径
        float border_width = 1.0f;       // 边框宽度
        float max_content_width = 400.0f; // 最大内容宽度
    } layout_;

    // 初始化方法
    void initializeColors();
    void initializeLayout();
    void setupClipboardManager();

    // 时间格式化
    std::string formatTime(const std::chrono::system_clock::time_point& time_point);
    std::string formatRelativeTime(const std::chrono::system_clock::time_point& time_point);

    // 内容处理
    std::string truncateContent(const std::string& content, size_t max_length);
    std::string highlightUrls(const std::string& content);
    bool hasLongContent(const ClipboardItem& item);

    // 统计信息
    void updateStatistics();
    struct Statistics {
        size_t total_items;
        size_t favorite_items;
        size_t total_urls;
        std::string last_update;
    } statistics_;
};

} // namespace DearTs::Core::Window::Widgets::Clipboard