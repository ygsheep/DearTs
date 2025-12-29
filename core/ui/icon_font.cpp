#include "icon_font.hpp"
#include "liblogger/logger.h"
#include <filesystem>

namespace DearTs {
namespace Core {
namespace UI {

// 静态成员初始化
ImFont* IconFont::s_icon_font = nullptr;
bool IconFont::s_loaded = false;

bool IconFont::loadFromFile(const char* font_path, float size_in_pixels, const ImWchar* icon_ranges) {
    ImGuiIO& io = ImGui::GetIO();
    IM_UNUSED(io);

    // 检查文件是否存在
    if (!std::filesystem::exists(font_path)) {
        LOG_WARN("图标字体文件不存在: {}", font_path);
        return false;
    }

    // 配置字体
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 加载图标字体
    s_icon_font = io.Fonts->AddFontFromFileTTF(font_path, size_in_pixels, &font_config, icon_ranges);

    if (s_icon_font == nullptr) {
        LOG_ERROR("加载图标字体失败: {}", font_path);
        s_loaded = false;
        return false;
    }

    LOG_INFO("成功加载图标字体: {} (大小: {:.1f}px)", font_path, size_in_pixels);
    s_loaded = true;
    return true;
}

bool IconFont::loadMaterialSymbols(float size_in_pixels) {
    // Material Symbols 图标范围
    // 支持的 Unicode 范围：U+E000–U+E8FF (Material Symbols Outlined)
    static const ImWchar MaterialSymbolsRanges[] = {
        0xE000, 0xE8FF, // Material Symbols Outlined
        0,
    };

    // 尝试多个可能的字体文件路径
    const char* possible_paths[] = {
        // 优先使用现有的 Material Symbols Rounded
        "resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
        "../resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
        "../../resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf",
        // 然后尝试 Material Symbols Outlined（如果有的话）
        "resources/fonts/MaterialSymbolsOutlined.woff2",
        "resources/fonts/MaterialSymbolsOutlined.ttf",
        "../resources/fonts/MaterialSymbolsOutlined.woff2",
        "../resources/fonts/MaterialSymbolsOutlined.ttf",
        "../../resources/fonts/MaterialSymbolsOutlined.woff2",
        "../../resources/fonts/MaterialSymbolsOutlined.ttf",
        "MaterialSymbolsOutlined.woff2",
        "MaterialSymbolsOutlined.ttf",
    };

    for (const char* path : possible_paths) {
        if (loadFromFile(path, size_in_pixels, MaterialSymbolsRanges)) {
            return true;
        }
    }

    LOG_WARN("未找到 Material Symbols 字体文件");
    return false;
}

ImFont* IconFont::getFont() {
    return s_icon_font;
}

bool IconFont::isLoaded() {
    return s_loaded;
}

const char* IconFont::MaterialIcon(const char* icon_name) {
    // 这里可以添加图标名称到 Unicode 的映射
    // 目前直接返回传入的字符串（假设是 Unicode 字符）
    return icon_name;
}

const char* IconFont::FontAwesome(const char* icon_name) {
    return icon_name;
}

} // namespace UI
} // namespace Core
} // namespace DearTs
