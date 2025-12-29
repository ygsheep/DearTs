#pragma once

#include <imgui.h>
#include <string>
#include <unordered_map>

namespace DearTs {
namespace Core {
namespace UI {

/**
 * @brief 图标字体管理器
 *
 * 支持多种图标字体：
 * - Material Symbols (Google)
 * - Font Awesome
 * - Codicons (VS Code)
 */
class IconFont {
public:
    /**
     * @brief 图标类型
     */
    enum class Type {
        Material,   // Material Symbols
        FontAwesome, // Font Awesome
        Codicon,    // VS Code Codicons
        Custom      // 自定义图标
    };

    /**
     * @brief 初始化图标字体
     * @param font_path 字体文件路径
     * @param size_in_pixels 字体大小（像素）
     * @param icon_ranges 图标字符范围
     * @return 是否成功加载
     */
    static bool loadFromFile(const char* font_path, float size_in_pixels, const ImWchar* icon_ranges);

    /**
     * @brief 使用内置图标字体
     *
     * 加载 Material Symbols 字体（如果字体文件存在）
     */
    static bool loadMaterialSymbols(float size_in_pixels = 18.0f);

    /**
     * @brief 获取图标字体指针
     */
    static ImFont* getFont();

    /**
     * @brief 检查图标字体是否已加载
     */
    static bool isLoaded();

    /**
     * @brief Material Icons 常用图标
     *
     * 完整图标列表：https://fonts.google.com/icons
     * 使用方式：ImGui::Text("%s", IconFont::MaterialIcon("home"));
     */
    static const char* MaterialIcon(const char* icon_name);

    /**
     * @brief Font Awesome 图标
     */
    static const char* FontAwesome(const char* icon_name);

private:
    static ImFont* s_icon_font;
    static bool s_loaded;
};

// ========== Material Icons 快捷访问 ==========
// UI 操作
#define ICON_HOME           ""
#define ICON_SPEED          ""
#define ICON_SETTINGS       ""
#define ICON_SEARCH         ""
#define ICON_MENU           ""
#define ICON_MORE_VERT      ""
#define ICON_CLOSE          ""
#define ICON_ADD            ""
#define ICON_REMOVE         ""
#define ICON_DELETE         ""
#define ICON_EDIT           ""
#define ICON_SAVE           ""
#define ICON_COPY           ""
#define ICON_CUT            ""
#define ICON_PASTE          ""

// 文件操作
#define ICON_FILE           ""
#define ICON_FOLDER         ""
#define ICON_FOLDER_OPEN    ""
#define ICON_DOWNLOAD       ""
#define ICON_UPLOAD         ""
#define ICON_DRIVE_FILE     ""

// 导航
#define ICON_ARROW_BACK     ""
#define ICON_ARROW_FORWARD  ""
#define ICON_EXPAND_MORE    ""
#define ICON_EXPAND_LESS    ""
#define ICON_KEYBOARD_ARROW_UP ""
#define ICON_KEYBOARD_ARROW_DOWN ""

// 状态
#define ICON_CHECK          ""
#define ICON_CANCEL         "✕"  // 使用简单的 X
#define ICON_WARNING        "⚠"
#define ICON_ERROR          ""
#define ICON_INFO           ""
#define ICON_SUCCESS        "✓"

// 视图
#define ICON_VIEW_LIST      ""
#define ICON_VIEW_MODULE    ""
#define ICON_GRID_VIEW      ""
#define ICON_FULLSCREEN     "⛶"
#define ICON_FULLSCREEN_EXIT "⛶"

// 代码相关
#define ICON_CODE           ""
#define ICON_TERMINAL       ""
#define ICON_CONSOLE        ""
#define ICON_DATABASE       ""
#define ICON_API            ""
#define ICON_PLUGIN         ""
#define ICON_EXTENSION      ""

// 系统
#define ICON_REFRESH        ""
#define ICON_SYNC           ""
#define ICON_LOCK           ""
#define ICON_UNLOCK         ""
#define ICON_VISIBILITY     ""
#define ICON_VISIBILITY_OFF ""

// 通信
#define ICON_EMAIL          ""
#define ICON_CHAT           ""
#define ICON_NOTIFICATION   ""
#define ICON_SHARE          ""
#define ICON_LINK           ""

// 媒体
#define ICON_IMAGE          ""
#define ICON_VIDEO          ""
#define ICON_AUDIO          ""
#define ICON_VOLUME_UP      ""
#define ICON_VOLUME_DOWN    ""
#define ICON_VOLUME_OFF     ""

// 用户
#define ICON_PERSON         "👤"
#define ICON_GROUP          "👥"
#define ICON_STAR           "★"
#define ICON_FAVORITE       "☆"
#define ICON_BOOKMARK       ""
#define ICON_BOOKMARK_BORDER ""

// 工具
#define ICON_BUILD          ""
#define ICON_HANDY          ""
#define ICON_TOOLS          ""
#define ICON_WRENCH         "🔧"
#define ICON_BUG_REPORT     ""

// 时间
#define ICON_SCHEDULE       ""
#define ICON_EVENT          ""
#define ICON_ALARM          "⏰"
#define ICON_ACCESS_TIME    ""
#define ICON_HISTORY       ""

// 调试
#define ICON_BUG            "🐛"
#define ICON_DEBUG          ""
#define ICON_LOGS           ""
#define ICON_TERMINAL_ICON  ""

// ========== Font Awesome Icons (备用) ==========
#define FA_ICON_HOME         "\uf015"
#define FA_ICON_SETTINGS     "\uf013"
#define FA_ICON_SEARCH       "\uf002"
#define FA_ICON_COG          "\uf013"
#define FA_ICON_TIMES        "\uf00d"
#define FA_ICON_CHECK        "\uf00c"
#define FA_ICON_PLAY         "\uf04b"
#define FA_ICON_PAUSE        "\uf04c"
#define FA_ICON_STOP         "\uf04d"

} // namespace UI
} // namespace Core
} // namespace DearTs
