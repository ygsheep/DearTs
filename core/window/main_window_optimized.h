#pragma once

#include "window_base.h"
#include "layouts/layout_manager.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include "layouts/pomodoro_layout.h"
#include "layouts/exchange_record_layout.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <imgui.h>

// 前向声明
namespace DearTs {
namespace Core {
namespace Window {
class SidebarLayout;
namespace Widgets { namespace Clipboard { class ClipboardHistoryLayout; } }
} // namespace Window
} // namespace Core
} // namespace DearTs

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 主窗口类 - 优化版本
 * 继承自WindowBase，移除冗余代码，简化架构
 */
class MainWindow : public WindowBase {
public:
    explicit MainWindow(const std::string& title = "DearTs Application");
    ~MainWindow() override;

    bool initialize() override;
    void render() override;
    void update() override;
    void handleEvent(const SDL_Event& event) override;

    // 简化的内容区域计算
    struct ContentArea {
        float x, y, width, height;
    };
    ContentArea getContentArea() const;

private:
    // 重要：保持与原始版本相同的成员变量结构
    ImVec4 clearColor_;  ///< 清屏颜色

    // 系统布局引用（直接访问，不拥有所有权）
    SidebarLayout* sidebarLayout_;

    // 剪切板监听器状态
    bool clipboard_monitoring_started_;

    // 简化的布局初始化
    void registerLayouts();
    void setupSidebarEventHandlers();
    void renderDefaultContent();
    void updateClipboardMonitoring();

    // 工具方法
    std::string mapSidebarItemToLayout(const std::string& itemId);
    void setupSidebarItems(SidebarLayout* sidebar);

    // RAII字体管理器
    class FontManager {
    public:
        FontManager();
        ~FontManager();
    private:
        std::shared_ptr<DearTs::Core::Resource::FontResource> font_;
    };
};

} // namespace Window
} // namespace Core
} // namespace DearTs