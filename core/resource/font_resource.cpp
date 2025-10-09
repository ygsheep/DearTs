/**
 * @file font_resource.cpp
 * @brief 字体资源管理实现
 * @author DearTs Team
 * @date 2025
 */

#include "font_resource.h"
#include "../utils/logger.h"
#include "../utils/file_utils.h"

#include <imgui.h>
#include <misc/freetype/imgui_freetype.h>
#include <iostream>
#include <algorithm>

namespace DearTs {
namespace Core {
namespace Resource {

// 静态成员初始化
FontManager* FontManager::instance_ = nullptr;

FontManager* FontManager::getInstance() {
    if (!instance_) {
        instance_ = new FontManager();
    }
    return instance_;
}

bool FontManager::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        // 清除现有字体
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        // 设置全局字体缩放，提升字体清晰度
        io.FontGlobalScale = 1.5f;  // 进一步放大以提升清晰度

        // 设置FreeType加载器
        #ifdef IMGUI_ENABLE_FREETYPE
        const ImFontLoader* freetypeLoader = ImGuiFreeType::GetFontLoader();
        if (freetypeLoader != nullptr) {
            io.Fonts->SetFontLoader(freetypeLoader);
        }
        #endif

        // 加载默认字体（使用15px作为全局默认字体大小）
        if (!loadDefaultFont(11.0f)) {
            DEARTS_LOG_ERROR("加载默认字体失败");
            return false;
        }
        
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
            DEARTS_LOG_ERROR("FontManager initialization failed: " + std::string(e.what()));
        return false;
    }
}

void FontManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    clearAll();
    initialized_ = false;
}

bool FontManager::loadDefaultFont(float fontSize, float scaleFactor) {
    try {
        ImGuiIO& io = ImGui::GetIO();
        
        // 获取可执行文件目录
        std::string exeDir = Utils::FileUtils::getExecutableDirectory();
        DEARTS_LOG_INFO("可执行文件目录: " + exeDir);
        
        // 构建字体文件的绝对路径
        std::string fontPath = "resources/fonts/OPPOSans-M.ttf";
        if (!exeDir.empty()) {
            fontPath = exeDir + "/" + fontPath;
            // 规范化路径
            fontPath = Utils::FileUtils::normalizePath(fontPath);
        }
        
        // 检查字体文件是否存在
        bool fontExists = Utils::FileUtils::exists(fontPath);
        DEARTS_LOG_INFO("检查字体文件: " + fontPath + ", 存在: " + (fontExists ? "是" : "否"));
        
        // 配置字体 - 优化FreeType渲染设置，进一步提升清晰度
        ImFontConfig config;
        config.SizePixels = fontSize * scaleFactor;
        config.OversampleH = 3;  // 提高水平采样率以增强清晰度
        config.OversampleV = 2;  // 提高垂直采样率以增强清晰度
        config.PixelSnapH = true;
        config.RasterizerMultiply = 1.0f;  // 避免过度加粗

        // 启用FreeType特定的优化设置，使用和demo相同的配置
        config.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;  // 禁用提示以保持字体原始设计，更适合中文字体
        strcpy_s(config.Name, sizeof(config.Name), "default");
        
        // 加载主字体（支持中文）
        ImFont* mainFont = nullptr;
        if (fontExists) {
            // 使用中文字符范围
            static const ImWchar chinese_ranges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x2000, 0x206F, // General Punctuation
                0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0xFF00, 0xFFEF, // Half-width characters
                0x4e00, 0x9FAF, // CJK Ideograms
                0,
            };
            
            mainFont = io.Fonts->AddFontFromFileTTF(
                fontPath.c_str(),
                fontSize * scaleFactor,
                &config,
                chinese_ranges
            );
        }
        
        if (!mainFont) {
            // 如果主字体加载失败，使用默认字体
            mainFont = io.Fonts->AddFontDefault();
            if (!mainFont) {
                DEARTS_LOG_ERROR("无法加载默认字体");
                return false;
            }
        }
        
        // 构建Material Symbols字体文件的绝对路径
        std::string materialSymbolsFontPath = "resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf";
        if (!exeDir.empty()) {
            materialSymbolsFontPath = exeDir + "/" + materialSymbolsFontPath;
            // 规范化路径
            materialSymbolsFontPath = Utils::FileUtils::normalizePath(materialSymbolsFontPath);
        }
        bool materialSymbolsFontExists = Utils::FileUtils::exists(materialSymbolsFontPath);
        DEARTS_LOG_INFO("检查Material Symbols字体文件: " + materialSymbolsFontPath + ", 存在: " + (materialSymbolsFontExists ? "是" : "否"));
        if (materialSymbolsFontExists) {
            ImFontConfig materialSymbolsConfig;
            materialSymbolsConfig.MergeMode = true;
            materialSymbolsConfig.PixelSnapH = true;
            materialSymbolsConfig.OversampleH = 2;
            materialSymbolsConfig.OversampleV = 1;
            materialSymbolsConfig.GlyphMinAdvanceX = fontSize * scaleFactor;
            // Material Symbols字体使用FreeType优化，和demo保持一致
            materialSymbolsConfig.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
            strcpy_s(materialSymbolsConfig.Name, sizeof(materialSymbolsConfig.Name), "material_symbols");
            
            // Material Symbols图标范围
            static const ImWchar material_symbols_ranges[] = { 0xe003, 0xf8ff, 0 }; // Material Symbols图标范围
            ImFont* materialSymbolsFont = io.Fonts->AddFontFromFileTTF(
                materialSymbolsFontPath.c_str(),
                fontSize * scaleFactor,
                &materialSymbolsConfig,
                material_symbols_ranges
            );
            
            if (materialSymbolsFont) {
                // 创建Material Symbols字体资源并添加到映射中
                FontConfig materialSymbolsFontConfig("material_symbols", materialSymbolsFontPath, fontSize, scaleFactor, material_symbols_ranges, true);
                auto materialSymbolsFontResource = std::make_shared<FontResource>(materialSymbolsFontPath, materialSymbolsFont, materialSymbolsFontConfig);
                fonts_["material_symbols"] = materialSymbolsFontResource;
                DEARTS_LOG_INFO("Material Symbols字体加载并存储成功");
            } else {
                DEARTS_LOG_WARN("从 " + materialSymbolsFontPath + " 加载Material Symbols字体失败");
            }
        } else {
            DEARTS_LOG_WARN("未找到Material Symbols字体: " + materialSymbolsFontPath);
        }
        
        // 构建Noto nerd字体文件的绝对路径
        std::string notoNerdFontPath = "resources/fonts/Noto nerd.ttf";
        if (!exeDir.empty()) {
            notoNerdFontPath = exeDir + "/" + notoNerdFontPath;
            // 规范化路径
            notoNerdFontPath = Utils::FileUtils::normalizePath(notoNerdFontPath);
        }
        bool notoNerdFontExists = Utils::FileUtils::exists(notoNerdFontPath);
        DEARTS_LOG_INFO("检查Noto nerd字体文件: " + notoNerdFontPath + ", 存在: " + (notoNerdFontExists ? "是" : "否"));
        if (notoNerdFontExists) {
            ImFontConfig notoNerdConfig;
            notoNerdConfig.MergeMode = true;
            notoNerdConfig.PixelSnapH = true;
            notoNerdConfig.GlyphMinAdvanceX = fontSize * scaleFactor;
            // Noto nerd字体使用FreeType优化，和demo保持一致
            notoNerdConfig.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
            strcpy_s(notoNerdConfig.Name, sizeof(notoNerdConfig.Name), "noto_nerd");
            
            // Nerd字体范围（包含各种图标和符号）
            static const ImWchar noto_nerd_ranges[] = { 
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x2000, 0x206F, // General Punctuation
                0x25A0, 0x25FF, // Geometric Shapes
                0x2B00, 0x2BFF, // Additional Arrows
                0x1F300, 0x1F5FF, // Miscellaneous Symbols and Pictographs
                0x1F600, 0x1F64F, // Emoticons
                0x1F680, 0x1F6FF, // Transport and Map Symbols
                0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
                0xE000, 0xF8FF, // Private Use Area (Nerd Fonts)
                0,
            };
            ImFont* notoNerdFont = io.Fonts->AddFontFromFileTTF(
                notoNerdFontPath.c_str(),
                fontSize * scaleFactor,
                &notoNerdConfig,
                noto_nerd_ranges
            );
            
            if (notoNerdFont) {
                // 创建Noto nerd字体资源并添加到映射中
                FontConfig notoNerdFontConfig("noto_nerd", notoNerdFontPath, fontSize, scaleFactor, noto_nerd_ranges, true);
                auto notoNerdFontResource = std::make_shared<FontResource>(notoNerdFontPath, notoNerdFont, notoNerdFontConfig);
                fonts_["noto_nerd"] = notoNerdFontResource;
                DEARTS_LOG_INFO("Noto nerd字体加载并存储成功");
            } else {
                DEARTS_LOG_WARN("从 " + notoNerdFontPath + " 加载Noto nerd字体失败");
            }
        } else {
            DEARTS_LOG_WARN("未找到Noto nerd字体: " + notoNerdFontPath);
        }
        
        // 创建字体资源
        FontConfig fontConfig("default", fontPath, fontSize, scaleFactor, io.Fonts->GetGlyphRangesChineseFull(), false);
        auto fontResource = std::make_shared<FontResource>(fontPath, mainFont, fontConfig);

        fonts_["default"] = fontResource;
        defaultFont_ = fontResource;
        currentScale_ = scaleFactor;

        // 设置为ImGui的全局默认字体，这样就不需要在每个布局中手动push/pop字体了
        io.FontDefault = mainFont;

        DEARTS_LOG_INFO("默认字体加载成功并设为全局默认字体");
        return true;
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("加载默认字体失败: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<FontResource> FontManager::loadLargeFont(float fontSize) {
    auto it = fonts_.find("large");
    if (it != fonts_.end()) {
        return it->second;
    }

    FontConfig config("large", "", fontSize, 1.0f, getChineseGlyphRanges(), false);
    return loadFontFromFile("large", "", config);
}

std::shared_ptr<FontResource> FontManager::loadTitleFont(float fontSize) {
    auto it = fonts_.find("title");
    if (it != fonts_.end()) {
        return it->second;
    }

    FontConfig config("title", "", fontSize, 1.0f, getChineseGlyphRanges(), false);
    return loadFontFromFile("title", "", config);
}

std::shared_ptr<FontResource> FontManager::loadFontFromFile(const std::string& name,
                                                           const std::string& path,
                                                           const FontConfig& config) {
    try {
        // 确定字体文件路径
        std::string fontPath = path;
        if (fontPath.empty()) {
            // 如果没有提供路径，使用默认字体路径
            std::string exeDir = Utils::FileUtils::getExecutableDirectory();
            fontPath = "resources/fonts/OPPOSans-M.ttf";
            if (!exeDir.empty()) {
                fontPath = exeDir + "/" + fontPath;
                fontPath = Utils::FileUtils::normalizePath(fontPath);
            }
        }

        // 检查文件是否存在
        if (!Utils::FileUtils::exists(fontPath)) {
            DEARTS_LOG_ERROR("Font file not found: " + fontPath);
            return nullptr;
        }
        
        // 检查是否已经加载
        auto it = fonts_.find(name);
        if (it != fonts_.end()) {
            return it->second;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        
        // 配置字体 - 使用优化的FreeType渲染设置
        ImFontConfig fontConfig;
        fontConfig.SizePixels = config.size * config.scale;
        fontConfig.MergeMode = config.mergeMode;
        fontConfig.OversampleH = 2;  // 提高水平采样率以增强清晰度
        fontConfig.OversampleV = 1;  // 保持垂直采样率为1以避免模糊
        fontConfig.PixelSnapH = true;
        fontConfig.RasterizerMultiply = 1.0f;  // 避免过度加粗
        // 启用FreeType优化
        fontConfig.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
        strcpy_s(fontConfig.Name, sizeof(fontConfig.Name), name.c_str());
        
        // 加载字体
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            fontPath.c_str(),
            fontConfig.SizePixels,
            &fontConfig,
            config.glyphRanges ? config.glyphRanges : getDefaultGlyphRanges()
        );
        
        if (!font) {
            return nullptr;
        }
        
        // 新的ImGui后端自动处理字体图集重建
        
        // 创建字体资源
        auto fontResource = std::make_shared<FontResource>(fontPath, font, config);
        fonts_[name] = fontResource;
        
        return fontResource;
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("FontManager::loadFontFromFile()：" + std::string(e.what()));
        return nullptr;
    }
}

std::shared_ptr<FontResource> FontManager::loadIconFont(const std::string& name,
                                                       const std::string& path,
                                                       float fontSize,
                                                       const ImWchar* iconRanges) {
    FontConfig config(name, path, fontSize, currentScale_, iconRanges, true);
    return loadFontFromFile(name, path, config);
}

std::shared_ptr<FontResource> FontManager::getFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second;
    }
    return nullptr;
}

bool FontManager::setDefaultFont(const std::string& name) {
    auto font = getFont(name);
    if (font) {
        defaultFont_ = font;
        // 设置为ImGui的全局默认字体
        ImGuiIO& io = ImGui::GetIO();
        io.FontDefault = font->getFont();
        DEARTS_LOG_INFO("全局默认字体已设置为: " + name);
        return true;
    }
    DEARTS_LOG_WARN("无法设置全局默认字体，字体不存在: " + name);
    return false;
}

std::shared_ptr<FontResource> FontManager::getDefaultFont() {
    return defaultFont_;
}


void FontManager::unloadFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        // 如果是默认字体，清除默认字体引用
        if (defaultFont_ == it->second) {
            defaultFont_ = nullptr;
        }
        
        fonts_.erase(it);

        // 新的ImGui后端自动处理字体图集重建
    }
}

void FontManager::clearAll() {
    fonts_.clear();
    defaultFont_ = nullptr;
    
    // 清除ImGui字体
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
}

const ImWchar* FontManager::getChineseGlyphRanges() {
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

const ImWchar* FontManager::getMaterialSymbolsGlyphRanges() {
    static const ImWchar ranges[] = {
        0xe003, 0xf8ff, // Material Symbols range
        0,
    };
    return &ranges[0];
}

void FontManager::setGlobalFontScale(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

float FontManager::getGlobalFontScale() const {
    ImGuiIO& io = ImGui::GetIO();
    return io.FontGlobalScale;
}

const ImWchar* FontManager::getDefaultGlyphRanges() {
    ImGuiIO& io = ImGui::GetIO();
    return io.Fonts->GetGlyphRangesDefault();
}

} // namespace Resource
} // namespace Core
} // namespace DearTs