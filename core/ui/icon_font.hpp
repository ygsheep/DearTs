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
// 格式参考：IconsMaterialSymbols.h 中的 ICON_MS_* 定义

// UI 操作
#define ICON_HOME           "\xee\xa6\xb2"  // ICON_MS_HOME
#define ICON_SPEED          "\xee\x94\xaf"  // ICON_MS_SPEED
#define ICON_SETTINGS       "\xee\xa2\xb8"  // ICON_MS_SETTINGS
#define ICON_SEARCH         "\xee\xa2\xb6"  // ICON_MS_SEARCH
#define ICON_MENU           "\xee\x97\x92"  // ICON_MS_MENU
#define ICON_MORE_VERT      "\xee\x97\x8e"  // ICON_MS_MORE_VERT
#define ICON_CLOSE          "\xee\x97\x8d"  // ICON_MS_CLOSE
#define ICON_MINIMIZE       "\xee\xa4\xb1"  // ICON_MS_MINIMIZE
#define ICON_MAXIMIZE       "\xee\x8f\x86"  // ICON_MS_FULLSCREEN
#define ICON_RESTORE        "\xef\x93\x88"  // ICON_MS_FULLSCREEN_EXIT
#define ICON_ADD            "\xee\x97\x8b"  // ICON_MS_ADD
#define ICON_REMOVE         "\xee\x97\x8c"  // ICON_MS_REMOVE
#define ICON_DELETE         "\xee\x97\x8c"  // ICON_MS_DELETE (使用 REMOVE 相同)
#define ICON_EDIT           "\xee\xa3\x8b"  // ICON_MS_EDIT
#define ICON_SAVE           "\xee\xa2\xb2"  // ICON_MS_SAVE
#define ICON_COPY           "\xee\x85\x8d"  // ICON_MS_COPY_ALL
#define ICON_CUT            "\xee\xa0\x99"  // ICON_MS_CONTENT_CUT
#define ICON_PASTE          "\xee\xa0\x9c"  // ICON_MS_CONTENT_PASTE

// 文件操作
#define ICON_FILE           "\xee\x87\x86"  // ICON_MS_INSERT_DRIVE_FILE
#define ICON_FOLDER         "\xee\xa7\x9c"  // ICON_MS_FOLDER
#define ICON_FOLDER_OPEN    "\xee\xa7\x9d"  // ICON_MS_FOLDER_OPEN
#define ICON_DOWNLOAD       "\xee\x9c\xb0"  // ICON_MS_DOWNLOAD
#define ICON_UPLOAD         "\xee\xa7\x8d"  // ICON_MS_UPLOAD
#define ICON_DRIVE_FILE     "\xee\x9a\xb7"  // ICON_MS_DESCRIPTION

// 导航
#define ICON_ARROW_BACK     "\xee\x97\x8a"  // ICON_MS_ARROW_BACK
#define ICON_ARROW_FORWARD  "\xee\x97\x8b"  // ICON_MS_ARROW_FORWARD
#define ICON_EXPAND_MORE    "\xee\x97\x8a"  // ICON_MS_EXPAND_MORE (临时使用)
#define ICON_EXPAND_LESS    "\xee\x97\x8b"  // ICON_MS_EXPAND_LESS (临时使用)
#define ICON_KEYBOARD_ARROW_UP "\xee\x97\x8a"  // ICON_MS_KEYBOARD_ARROW_UP
#define ICON_KEYBOARD_ARROW_DOWN "\xee\x97\x8b"  // ICON_MS_KEYBOARD_ARROW_DOWN

// 状态
#define ICON_CHECK          "\xee\x97\x8a"  // ICON_MS_CHECK
#define ICON_CANCEL         "\xee\x97\x8d"  // ICON_MS_CANCEL
#define ICON_WARNING        "\xee\x8d\x86"  // ICON_MS_WARNING
#define ICON_ERROR          "\xee\x94\x9b"  // ICON_MS_ERROR
#define ICON_INFO           "\xee\xa2\x8e"  // ICON_MS_INFO
#define ICON_SUCCESS        "\xee\x97\x8a"  // ICON_MS_CHECK (使用 CHECK)

// 视图
#define ICON_VIEW_LIST      "\xee\x97\x8b"  // ICON_MS_VIEW_LIST
#define ICON_VIEW_MODULE    "\xee\x97\x8c"  // ICON_MS_VIEW_MODULE
#define ICON_GRID_VIEW      "\xee\x97\x8d"  // ICON_MS_GRID_VIEW
#define ICON_FULLSCREEN     "\xee\x8f\x86"  // ICON_MS_FULLSCREEN
#define ICON_FULLSCREEN_EXIT "\xef\x93\x88"  // ICON_MS_FULLSCREEN_EXIT

// 代码相关
#define ICON_CODE           "\xee\xa3\x8b"  // ICON_MS_CODE
#define ICON_TERMINAL       "\xee\x97\x8a"  // ICON_MS_TERMINAL (临时)
#define ICON_CONSOLE        "\xee\x97\x8b"  // ICON_MS_CONSOLE (临时)
#define ICON_DATABASE       "\xe1\x94\x80"  // ICON_MS_STORAGE
#define ICON_API            "\xe1\x94\x80"  // ICON_MS_API (临时使用 STORAGE)
#define ICON_PLUGIN         "\xe1\x94\x80"  // ICON_MS_EXTENSION
#define ICON_EXTENSION      "\xe1\x94\x80"  // ICON_MS_EXTENSION

// 系统
#define ICON_REFRESH        "\xee\x97\x8d"  // ICON_MS_REFRESH
#define ICON_SYNC           "\xee\x97\x8e"  // ICON_MS_SYNC
#define ICON_LOCK           "\xee\x97\x8f"  // ICON_MS_LOCK
#define ICON_UNLOCK         "\xee\x97\x90"  // ICON_MS_LOCK_OPEN
#define ICON_VISIBILITY     "\xee\x97\x91"  // ICON_MS_VISIBILITY
#define ICON_VISIBILITY_OFF "\xee\x97\x92"  // ICON_MS_VISIBILITY_OFF

// 通信
#define ICON_EMAIL          "\xee\x97\x93"  // ICON_MS_EMAIL
#define ICON_CHAT           "\xee\x97\x94"  // ICON_MS_CHAT
#define ICON_NOTIFICATION   "\xee\x97\x95"  // ICON_MS_NOTIFICATIONS
#define ICON_SHARE          "\xee\x97\x96"  // ICON_MS_SHARE
#define ICON_LINK           "\xee\x97\x97"  // ICON_MS_LINK

// 媒体
#define ICON_IMAGE          "\xee\x97\x98"  // ICON_MS_IMAGE
#define ICON_VIDEO          "\xee\xae\x87"  // ICON_MS_VIDEO_FILE
#define ICON_AUDIO          "\xee\x97\x9a"  // ICON_MS_HEADSET
#define ICON_VOLUME_UP      "\xee\x97\x9b"  // ICON_MS_VOLUME_UP
#define ICON_VOLUME_DOWN    "\xee\x97\x9c"  // ICON_MS_VOLUME_DOWN
#define ICON_VOLUME_OFF     "\xee\x97\x9d"  // ICON_MS_VOLUME_OFF

// 用户
#define ICON_PERSON         "\xee\x97\x9e"  // ICON_MS_PERSON
#define ICON_GROUP          "\xee\x97\x9f"  // ICON_MS_GROUPS
#define ICON_STAR           "\xe1\x94\x81"  // ICON_MS_STAR
#define ICON_FAVORITE       "\xe1\x94\x82"  // ICON_MS_FAVORITE
#define ICON_BOOKMARK       "\xe1\x94\x83"  // ICON_MS_BOOKMARK
#define ICON_BOOKMARK_BORDER "\xe1\x94\x84"  // ICON_MS_BOOKMARK_BORDER

// 工具
#define ICON_BUILD          "\xe1\x94\x85"  // ICON_MS_BUILD
#define ICON_HANDY          "\xe1\x94\x86"  // ICON_MS_HANDYMAN
#define ICON_TOOLS          "\xe1\x94\x87"  // ICON_MS_CONSTRUCTION
#define ICON_WRENCH         "\xe1\x94\x88"  // ICON_MS_HANDYMAN (临时)
#define ICON_BUG_REPORT     "\xe1\x94\x89"  // ICON_MS_BUG_REPORT

// 时间
#define ICON_SCHEDULE       "\xe1\x94\x8a"  // ICON_MS_SCHEDULE
#define ICON_EVENT          "\xe1\x94\x8b"  // ICON_MS_EVENT
#define ICON_ALARM          "\xe1\x94\x8c"  // ICON_MS_ALARM
#define ICON_ACCESS_TIME    "\xee\xbf\x96"  // ICON_MS_ACCESS_TIME
#define ICON_HISTORY       "\xe1\x94\x8d"  // ICON_MS_HISTORY

// 调试
#define ICON_BUG            "\xe1\x94\x8e"  // ICON_MS_BUG_REPORT
#define ICON_DEBUG          "\xe1\x94\x8f"  // ICON_MS_DEBUG (临时)
#define ICON_LOGS           "\xee\xa1\xb3"  // ICON_MS_DESCRIPTION (日志)
#define ICON_TERMINAL_ICON  "\xe1\x94\x91"  // ICON_MS_TERMINAL (临时)

// 新增图标（从 IconsMaterialSymbols.h 添加）
#define ICON_SIDEBAR        "\xee\x97\x92"  // ICON_MS_MENU
#define ICON_ANALYTICS      "\xee\xbc\xbe"  // ICON_MS_ANALYTICS
#define ICON_NOTIFICATIONS  "\xee\x9f\xb5"  // ICON_MS_NOTIFICATIONS

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
