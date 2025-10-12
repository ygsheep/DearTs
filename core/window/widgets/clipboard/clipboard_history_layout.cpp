#include "clipboard_history_layout.h"
#include "clipboard_monitor.h"
#include "../../utils/logger.h"
#include "../../resource/IconsMaterialSymbols.h"
#include <SDL_syswm.h>
#include <algorithm>

namespace DearTs::Core::Window::Widgets::Clipboard {

ClipboardHistoryLayout::ClipboardHistoryLayout()
    : LayoutBase("ClipboardHistory")
    , selected_item_index_(-1)
    , hovered_item_index_(-1)
    , show_favorites_only_(false)
    , search_focused_(false) {

    DEARTS_LOG_INFO("ClipboardHistoryLayout构造函数");

    // 初始化搜索缓冲区
    memset(search_buffer_, 0, sizeof(search_buffer_));

    initializeColors();
    initializeLayout();
    setupClipboardManager();
}

ClipboardHistoryLayout::~ClipboardHistoryLayout() {
    DEARTS_LOG_INFO("ClipboardHistoryLayout析构函数");
}

void ClipboardHistoryLayout::initializeColors() {
    colors_.window_bg = ImVec4(0.15f, 0.15f, 0.15f, 0.85f);
    colors_.header_bg = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
    colors_.item_normal = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
    colors_.item_hovered = ImVec4(0.2f, 0.2f, 0.3f, 0.9f);
    colors_.item_selected = ImVec4(0.3f, 0.3f, 0.5f, 0.9f);
    colors_.item_favorite = ImVec4(0.4f, 0.3f, 0.2f, 0.9f);
    colors_.text_normal = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors_.text_dimmed = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    colors_.text_url = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
    colors_.border_normal = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors_.border_hovered = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    colors_.border_selected = ImVec4(0.6f, 0.6f, 0.8f, 1.0f);
    colors_.search_bg = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
    colors_.button_normal = ImVec4(0.2f, 0.2f, 0.2f, 0.8f);
    colors_.button_hovered = ImVec4(0.3f, 0.3f, 0.3f, 0.9f);
}

void ClipboardHistoryLayout::initializeLayout() {
    window_size_ = ImVec2(500, 600);
    content_margin_ = ImVec2(10.0f, 10.0f);
}

void ClipboardHistoryLayout::setupClipboardManager() {
    // 创建剪切板管理器
    DEARTS_LOG_INFO("设置剪切板管理器");

    // 注意：分词窗口现在由GUI应用程序统一管理，不再在此处创建

    // 设置剪切板监听器回调
    auto& monitor = ClipboardMonitor::getInstance();
    monitor.setChangeCallback([this](const std::string& content) {
        this->onClipboardContentChanged(content);
    });

    // 初始化过滤列表
    filtered_items_ = history_items_;

    DEARTS_LOG_INFO("剪切板管理器设置完成，监听器回调已设置");
}

void ClipboardHistoryLayout::render() {
    // 作为布局组件在父窗口中渲染内容，不创建自己的ImGui窗口
    // 渲染各个组件
    renderHeader();
    renderSearchBox();
    renderFilterBar();
    renderHistoryList();
    renderFooter();

    // 处理交互
    handleMouseInteraction();
    handleKeyboardInput();

    // 注意：分词窗口现在由WindowManager管理，不再在这里直接渲染
}

void ClipboardHistoryLayout::renderTranslucentBackground() {
    // 作为组件，不需要设置背景，父窗口会处理
    // ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
}

void ClipboardHistoryLayout::renderHeader() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors_.header_bg);

    if (ImGui::BeginChild("Header", ImVec2(0, layout_.header_height), true)) {
        if (ImGui::Button(ICON_MS_REFRESH " 刷新")) {
            refreshHistory();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_MS_DELETE " 清空")) {
            clearHistory();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_MS_CONTENT_COPY " 导出")) {
            exportHistory();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_MS_FORMAT_TEXT_CLIP " 分词助手")) {
            toggleSegmentationWindow();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ClipboardHistoryLayout::renderSearchBox() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors_.search_bg);

    if (ImGui::BeginChild("Search", ImVec2(0, layout_.search_height), true)) {
        if (ImGui::InputText(ICON_MS_SEARCH " 搜索", search_buffer_, sizeof(search_buffer_))) {
            searchItems(std::string(search_buffer_));
            last_search_keyword_ = search_buffer_;
        }

        if (ImGui::IsItemClicked()) {
            search_focused_ = true;
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ClipboardHistoryLayout::renderFilterBar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

    if (ImGui::BeginChild("Filter", ImVec2(0, layout_.filter_height), true)) {
        if (ImGui::Button("全部")) {
            show_favorites_only_ = false;
            current_filter_.clear();
            updateFilteredList();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_MS_STAR " 收藏")) {
            show_favorites_only_ = !show_favorites_only_;
            updateFilteredList();
        }

        ImGui::SameLine();
        ImGui::Text("共 %d 项", filtered_items_.size());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ClipboardHistoryLayout::renderHistoryList() {
    float remaining_height = ImGui::GetContentRegionAvail().y - layout_.footer_height;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.2f));

    if (ImGui::BeginChild("HistoryList", ImVec2(0, remaining_height), true)) {
        if (filtered_items_.empty()) {
            ImGui::TextColored(colors_.text_dimmed, "暂无剪切板记录");
            ImGui::TextColored(colors_.text_dimmed, "复制内容后会自动显示在这里");
        } else {
            for (size_t i = 0; i < filtered_items_.size(); ++i) {
                renderHistoryItem(filtered_items_[i], static_cast<int>(i));
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ClipboardHistoryLayout::renderHistoryItem(const ClipboardItem& item, int index) {
    bool is_selected = (index == selected_item_index_);
    bool is_hovered = (index == hovered_item_index_);

    // 设置项目颜色
    ImVec4 bg_color = colors_.item_normal;
    ImVec4 border_color = colors_.border_normal;

    if (item.is_favorite) {
        bg_color = colors_.item_favorite;
    }

    if (is_selected) {
        bg_color = colors_.item_selected;
        border_color = colors_.border_selected;
    } else if (is_hovered) {
        bg_color = colors_.item_hovered;
        border_color = colors_.border_hovered;
    }

    // 设置按钮样式
    ImGui::PushStyleColor(ImGuiCol_Button, bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors_.item_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors_.item_selected);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, is_selected ? 2.0f : 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(layout_.item_padding, layout_.item_padding));

    // 渲染项目按钮
    std::string button_label = "项目 " + std::to_string(index + 1);
    if (ImGui::Button(button_label.c_str(), ImVec2(-1, layout_.item_min_height))) {
        selected_item_index_ = index;
        selected_item_id_ = item.id;

        // 双击打开分词窗口
        static auto last_click = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (now - last_click < std::chrono::milliseconds(500)) {
            openSegmentationWindow(item);
        }
        last_click = now;
    }

    // 绘制边框
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 button_min = ImGui::GetItemRectMin();
    ImVec2 button_max = ImGui::GetItemRectMax();
    draw_list->AddRect(button_min, button_max,
                      ImGui::ColorConvertFloat4ToU32(border_color),
                      layout_.corner_radius, 0, 1.0f);

    // 恢复样式
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    // 渲染项目内容
    renderItemContent(item);

    // 渲染项目操作
    renderItemActions(item, index);
}

void ClipboardHistoryLayout::renderItemContent(const ClipboardItem& item) {
    ImGui::SameLine();

    // 显示收藏标记
    if (item.is_favorite) {
        ImGui::Text(ICON_MS_STAR " ");
    }

    // 显示时间
    std::string time_str = formatTime(item.timestamp);
    ImGui::TextColored(colors_.text_dimmed, "%s", time_str.c_str());
    ImGui::SameLine();

    // 显示内容预览
    std::string preview = truncateContent(item.content, 50);
    ImGui::TextWrapped("%s", preview.c_str());

    // 显示URL信息
    if (!item.urls.empty()) {
        ImGui::TextColored(colors_.text_url, "🔗 %d 个链接", item.urls.size());
    }
}

void ClipboardHistoryLayout::renderItemActions(const ClipboardItem& item, int index) {
    ImGui::SameLine();

    // 复制按钮
    if (ImGui::Button((ICON_MS_CONTENT_PASTE "##copy_" + std::to_string(index)).c_str())) {
        copySelectedItem();
        DEARTS_LOG_DEBUG("点击复制按钮，项目索引: " + std::to_string(index));
    }

    ImGui::SameLine();

    // 分词分析按钮
    if (ImGui::Button((ICON_MS_FORMAT_TEXT_CLIP "##segment_" + std::to_string(index)).c_str())) {
        openSegmentationWindow(item);
        DEARTS_LOG_DEBUG("点击分词按钮，项目索引: " + std::to_string(index));
    }

    ImGui::SameLine();

    // 收藏按钮
    if (ImGui::Button((ICON_MS_STAR "##favorite_" + std::to_string(index)).c_str())) {
        toggleFavoriteItem();
        DEARTS_LOG_DEBUG("点击收藏按钮，项目索引: " + std::to_string(index));
    }

    ImGui::SameLine();

    // 删除按钮
    if (ImGui::Button((ICON_MS_DELETE "##delete_" + std::to_string(index)).c_str())) {
        deleteSelectedItem();
        DEARTS_LOG_DEBUG("点击删除按钮，项目索引: " + std::to_string(index));
    }
}

void ClipboardHistoryLayout::renderFooter() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));

    if (ImGui::BeginChild("Footer", ImVec2(0, layout_.footer_height), true)) {
        ImGui::Text("状态: 就绪 | 项目: %zu", filtered_items_.size());

        ImGui::SameLine();
        if (selected_item_index_ >= 0) {
            ImGui::Text(" | 已选择: 项目 %d", selected_item_index_ + 1);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ClipboardHistoryLayout::handleMouseInteraction() {
    ImVec2 mouse_pos = ImGui::GetMousePos();

    // 重置悬停状态
    hovered_item_index_ = -1;

    // 检查历史项目悬停
    // 这里可以添加更精确的鼠标位置检测逻辑
}

void ClipboardHistoryLayout::handleKeyboardInput() {
    // 处理键盘快捷键
    const Uint8* state = SDL_GetKeyboardState(nullptr);

    if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
        if (state[SDL_SCANCODE_F]) {
            search_focused_ = true;
        } else if (state[SDL_SCANCODE_A]) {
            // 全选功能
        }
    }

    if (state[SDL_SCANCODE_ESCAPE]) {
        hideWindow();
    }

    if (state[SDL_SCANCODE_DELETE] && selected_item_index_ >= 0) {
        deleteSelectedItem();
    }
}

void ClipboardHistoryLayout::handleSearchInput() {
    // 搜索输入处理
}

void ClipboardHistoryLayout::handleFilterSelection() {
    // 过滤选择处理
}

void ClipboardHistoryLayout::handleItemDoubleClick() {
    // 双击处理
}

void ClipboardHistoryLayout::handleContextMenu() {
    // 右键菜单处理
}

void ClipboardHistoryLayout::updateFilteredList() {
    // 更新过滤列表
    filtered_items_ = history_items_;

    // 应用收藏过滤
    if (show_favorites_only_) {
        filtered_items_.erase(
            std::remove_if(filtered_items_.begin(), filtered_items_.end(),
                          [](const ClipboardItem& item) { return !item.is_favorite; }),
            filtered_items_.end()
        );
    }

    // 应用搜索过滤
    if (!last_search_keyword_.empty()) {
        filtered_items_.erase(
            std::remove_if(filtered_items_.begin(), filtered_items_.end(),
                          [this](const ClipboardItem& item) {
                              return item.content.find(last_search_keyword_) == std::string::npos;
                          }),
            filtered_items_.end()
        );
    }
}

void ClipboardHistoryLayout::searchItems(const std::string& keyword) {
    last_search_keyword_ = keyword;
    updateFilteredList();
}

void ClipboardHistoryLayout::filterByCategory(const std::string& category) {
    current_filter_ = category;
    updateFilteredList();
}

void ClipboardHistoryLayout::toggleFavorites() {
    show_favorites_only_ = !show_favorites_only_;
    updateFilteredList();
}

void ClipboardHistoryLayout::copySelectedItem() {
    if (selected_item_index_ >= 0 && selected_item_index_ < static_cast<int>(filtered_items_.size())) {
        const auto& item = filtered_items_[selected_item_index_];

        if (OpenClipboard(nullptr)) {
            EmptyClipboard();

            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, item.content.length() + 1);
            if (hClipboardData) {
                char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
                if (pchData) {
                    strcpy_s(pchData, item.content.length() + 1, item.content.c_str());
                    GlobalUnlock(hClipboardData);
                    SetClipboardData(CF_TEXT, hClipboardData);
                    DEARTS_LOG_INFO("复制剪切板内容: " + item.content.substr(0, 50) + "...");
                }
            }
            CloseClipboard();
        }
    }
}

void ClipboardHistoryLayout::deleteSelectedItem() {
    if (selected_item_index_ >= 0 && selected_item_index_ < static_cast<int>(filtered_items_.size())) {
        // 删除选中的项目
        DEARTS_LOG_INFO("删除剪切板项目: " + std::to_string(selected_item_index_));
        updateFilteredList();
        selected_item_index_ = -1;
    }
}

void ClipboardHistoryLayout::toggleFavoriteItem() {
    if (selected_item_index_ >= 0 && selected_item_index_ < static_cast<int>(filtered_items_.size())) {
        // 切换收藏状态
        DEARTS_LOG_INFO("切换收藏状态: " + std::to_string(selected_item_index_));
        updateFilteredList();
    }
}

void ClipboardHistoryLayout::openSegmentationWindow(const ClipboardItem& item) {
    // 分词窗口功能已被移除，只记录日志
    DEARTS_LOG_INFO("分词窗口功能已被移除，不再支持");
    DEARTS_LOG_DEBUG("剪切板内容预览: " + item.content.substr(0, std::min(50, static_cast<int>(item.content.length()))) + "...");
}

void ClipboardHistoryLayout::exportHistory() {
    // 导出历史记录
    DEARTS_LOG_INFO("导出剪切板历史记录");
}

void ClipboardHistoryLayout::importHistory() {
    // 导入历史记录
    DEARTS_LOG_INFO("导入剪切板历史记录");
}

void ClipboardHistoryLayout::updateLayout(float width, float height) {
    setSize(width, height);
    window_size_ = ImVec2(width, height);
    calculateLayout();
}

void ClipboardHistoryLayout::handleEvent(const SDL_Event& event) {
    // 处理SDL事件
}

void ClipboardHistoryLayout::renderInFixedArea(float contentX, float contentY, float contentWidth, float contentHeight) {
    // 在固定区域内渲染剪切板历史记录
    // 使用提供的固定区域，避免浮动窗口

    // 计算内容区域，留出边距
    const float padding = 15.0f;
    const float startX = contentX + padding;
    const float startY = contentY + padding;
    const float availableWidth = contentWidth - padding * 2;
    const float availableHeight = contentHeight - padding * 2;

    // 设置内容位置
    ImGui::SetCursorScreenPos(ImVec2(startX, startY));

    // 渲染各个组件，使用固定宽度
    renderHeader();
    renderSearchBox();
    renderFilterBar();
    renderHistoryList();
    renderFooter();

    // 处理交互
    handleMouseInteraction();
    handleKeyboardInput();

    // 注意：分词窗口现在由WindowManager管理，不再在这里直接渲染
}

void ClipboardHistoryLayout::showWindow() {
    is_visible_ = true;
    DEARTS_LOG_INFO("显示剪切板历史窗口");
}

void ClipboardHistoryLayout::hideWindow() {
    is_visible_ = false;
    DEARTS_LOG_INFO("隐藏剪切板历史窗口");
}

void ClipboardHistoryLayout::toggleWindow() {
    if (is_visible_) {
        hideWindow();
    } else {
        showWindow();
    }
}

void ClipboardHistoryLayout::refreshHistory() {
    // 刷新历史记录
    DEARTS_LOG_INFO("刷新剪切板历史记录");

    // 这里可以添加实际的刷新逻辑
    updateFilteredList();
}

void ClipboardHistoryLayout::clearHistory() {
    // 清空历史记录
    DEARTS_LOG_INFO("清空剪切板历史记录");

    history_items_.clear();
    filtered_items_.clear();
    selected_item_index_ = -1;
    hovered_item_index_ = -1;
}

void ClipboardHistoryLayout::onClipboardContentChanged(const std::string& content) {
    DEARTS_LOG_INFO("接收到剪切板内容变化: " + std::to_string(content.length()) + " 字符");

    // 创建新的剪切板项目
    ClipboardItem item(content);
    item.id = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    item.is_favorite = false;

    // 检查是否重复
    for (const auto& existing_item : history_items_) {
        if (existing_item.content == content) {
            DEARTS_LOG_DEBUG("剪切板内容已存在，跳过添加");
            return;
        }
    }

    // 添加到历史记录
    history_items_.insert(history_items_.begin(), item);

    // 限制历史记录数量
    const size_t max_history = 100;
    if (history_items_.size() > max_history) {
        history_items_.resize(max_history);
    }

    // 更新过滤列表
    updateFilteredList();

    DEARTS_LOG_INFO("已添加新剪切板项目，当前历史记录数: " + std::to_string(history_items_.size()));
}

void ClipboardHistoryLayout::startClipboardMonitoring(SDL_Window* sdl_window) {
    if (!sdl_window) {
        DEARTS_LOG_ERROR("无效的SDL窗口句柄");
        return;
    }

    // 获取Windows窗口句柄
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(sdl_window, &wmInfo) == 0 && wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
        HWND hwnd = wmInfo.info.win.window;

        auto& monitor = ClipboardMonitor::getInstance();
        if (monitor.startMonitoring(hwnd)) {
            DEARTS_LOG_INFO("剪切板监听启动成功");

            // 获取当前剪切板内容作为初始内容
            std::string current_content = monitor.getCurrentClipboardContent();
            if (!current_content.empty()) {
                onClipboardContentChanged(current_content);
            }
        } else {
            DEARTS_LOG_ERROR("剪切板监听启动失败");
        }
    } else {
        DEARTS_LOG_ERROR("无法获取Windows窗口句柄");
    }
}

void ClipboardHistoryLayout::setSelectedItem(const std::string& id) {
    // 设置选中的项目
    for (size_t i = 0; i < filtered_items_.size(); ++i) {
        if (filtered_items_[i].id == id) {
            selected_item_index_ = static_cast<int>(i);
            selected_item_id_ = id;
            break;
        }
    }
}

ImVec2 ClipboardHistoryLayout::calculateItemSize(const ClipboardItem& item) {
    // 计算项目大小
    return ImVec2(ImGui::GetContentRegionAvail().x - 20.0f, layout_.item_min_height);
}

void ClipboardHistoryLayout::arrangeItems() {
    // 排列项目
    calculateLayout();
}

void ClipboardHistoryLayout::calculateLayout() {
    // 计算布局
}

void ClipboardHistoryLayout::handleShortcuts() {
    // 处理快捷键
}

std::string ClipboardHistoryLayout::formatTime(const std::chrono::system_clock::time_point& time_point) {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm = *std::localtime(&time_t);

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);

    return std::string(buffer);
}

std::string ClipboardHistoryLayout::formatRelativeTime(const std::chrono::system_clock::time_point& time_point) {
    auto now = std::chrono::system_clock::now();
    auto diff = now - time_point;

    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
    auto days = std::chrono::duration_cast<std::chrono::days>(diff).count();

    if (days > 0) {
        return std::to_string(days) + "天前";
    } else if (hours > 0) {
        return std::to_string(hours) + "小时前";
    } else if (minutes > 0) {
        return std::to_string(minutes) + "分钟前";
    } else {
        return "刚刚";
    }
}

std::string ClipboardHistoryLayout::truncateContent(const std::string& content, size_t max_length) {
    if (content.length() <= max_length) {
        return content;
    }

    return content.substr(0, max_length - 3) + "...";
}

std::string ClipboardHistoryLayout::highlightUrls(const std::string& content) {
    // 高亮URL
    return content;
}

bool ClipboardHistoryLayout::hasLongContent(const ClipboardItem& item) {
    return item.content.length() > 100;
}

void ClipboardHistoryLayout::updateStatistics() {
    // 更新统计信息
    statistics_.total_items = history_items_.size();
    statistics_.favorite_items = std::count_if(history_items_.begin(), history_items_.end(),
                                          [](const ClipboardItem& item) { return item.is_favorite; });
    statistics_.total_urls = 0;
    for (const auto& item : history_items_) {
        statistics_.total_urls += item.urls.size();
    }
    statistics_.last_update = formatTime(std::chrono::system_clock::now());
}

void ClipboardHistoryLayout::toggleSegmentationWindow() {
    // 分词窗口功能已被移除，只记录日志
    DEARTS_LOG_INFO("分词窗口功能已被移除，不再支持切换操作");

    // 如果有选中的剪切板项目，记录内容信息
    if (selected_item_index_ >= 0 && selected_item_index_ < static_cast<int>(filtered_items_.size())) {
        const auto& item = filtered_items_[selected_item_index_];
        DEARTS_LOG_DEBUG("选中剪切板内容预览: " + item.content.substr(0, std::min(50, static_cast<int>(item.content.length()))) + "...");
    }
}

} // namespace DearTs::Core::Window::Widgets::Clipboard