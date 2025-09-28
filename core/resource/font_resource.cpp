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
        
        // 加载默认字体
        if (!loadDefaultFont()) {
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
        io.Fonts->Clear();
        
        // 获取可执行文件目录，用于构建字体文件的绝对路径
        std::string exeDir = Utils::FileUtils::getExecutableDirectory();
        DEARTS_LOG_INFO("可执行文件目录: " + exeDir);
        
        // 构建主字体文件的绝对路径
        std::string fontPath = "resources/fonts/OPPOSans-M.ttf";
        if (!exeDir.empty()) {
            fontPath = exeDir + "/" + fontPath;
            // 规范化路径
            fontPath = Utils::FileUtils::normalizePath(fontPath);
        }
        bool fontExists = Utils::FileUtils::exists(fontPath);
        DEARTS_LOG_INFO("检查主字体文件: " + fontPath + ", 存在: " + (fontExists ? "是" : "否"));
        
        // 配置主字体
        ImFontConfig config;
        config.SizePixels = fontSize * scaleFactor;
        config.OversampleH = 2;
        config.OversampleV = 1;
        strcpy_s(config.Name, sizeof(config.Name), "MainFont");
        
        // 加载主字体（中文支持）
        ImFont* mainFont = nullptr;
        if (fontExists) {
            mainFont = io.Fonts->AddFontFromFileTTF(
                fontPath.c_str(),
                fontSize * scaleFactor,
                &config,
                io.Fonts->GetGlyphRangesChineseFull()
            );
        }
        
        if (!mainFont) {
            // 如果主字体加载失败，回退到默认字体
            DEARTS_LOG_WARN("从 " + fontPath + " 加载主字体失败，回退到默认字体");
            mainFont = io.Fonts->AddFontDefault(&config);
            if (!mainFont) {
                DEARTS_LOG_ERROR("加载默认字体失败");
                return false;
            }
        }
        
        // 构建图标字体文件的绝对路径
        std::string iconFontPath = "resources/fonts/codicons.ttf";
        if (!exeDir.empty()) {
            iconFontPath = exeDir + "/" + iconFontPath;
            // 规范化路径
            iconFontPath = Utils::FileUtils::normalizePath(iconFontPath);
        }
        bool iconFontExists = Utils::FileUtils::exists(iconFontPath);
        DEARTS_LOG_INFO("检查图标字体文件: " + iconFontPath + ", 存在: " + (iconFontExists ? "是" : "否"));
        if (iconFontExists) {
            ImFontConfig iconConfig;
            iconConfig.MergeMode = true;
            iconConfig.PixelSnapH = true;
            iconConfig.GlyphMinAdvanceX = fontSize * scaleFactor;
            strcpy_s(iconConfig.Name, sizeof(iconConfig.Name), "icons");
            
            // 使用正确的VS Code图标范围
            static const ImWchar icon_ranges[] = { 0xea60, 0xec25, 0 }; // VS Code图标范围
            ImFont* iconFont = io.Fonts->AddFontFromFileTTF(
                iconFontPath.c_str(),
                fontSize * scaleFactor,
                &iconConfig,
                icon_ranges
            );
            
            if (iconFont) {
                // 创建图标字体资源并添加到映射中
                FontConfig iconFontConfig("icons", iconFontPath, fontSize, scaleFactor, icon_ranges, true);
                auto iconFontResource = std::make_shared<FontResource>(iconFontPath, iconFont, iconFontConfig);
                fonts_["icons"] = iconFontResource;
                DEARTS_LOG_INFO("图标字体加载并存储成功");
            } else {
                DEARTS_LOG_WARN("从 " + iconFontPath + " 加载图标字体失败");
            }
        } else {
            DEARTS_LOG_WARN("未找到图标字体: " + iconFontPath);
        }
        
        // 重建字体图集
        if (!rebuildFontAtlas()) {
            DEARTS_LOG_ERROR("重建字体图集失败");
            return false;
        }
        
        // 创建字体资源
        FontConfig fontConfig("default", fontPath, fontSize, scaleFactor, io.Fonts->GetGlyphRangesChineseFull(), false);
        auto fontResource = std::make_shared<FontResource>(fontPath, mainFont, fontConfig);
        
        fonts_["default"] = fontResource;
        defaultFont_ = fontResource;
        currentScale_ = scaleFactor;
        
        DEARTS_LOG_INFO("默认字体加载成功");
        return true;
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("加载默认字体失败: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<FontResource> FontManager::loadFontFromFile(const std::string& name,
                                                           const std::string& path,
                                                           const FontConfig& config) {
    try {
        // 检查文件是否存在
        if (!Utils::FileUtils::exists(path)) {
            DEARTS_LOG_ERROR("Font file not found: " + path);
            return nullptr;
        }
        
        // 检查是否已经加载
        auto it = fonts_.find(name);
        if (it != fonts_.end()) {
            return it->second;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        
        // 配置字体
        ImFontConfig fontConfig;
        fontConfig.SizePixels = config.size * config.scale;
        fontConfig.MergeMode = config.mergeMode;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 1;
        strcpy_s(fontConfig.Name, sizeof(fontConfig.Name), name.c_str());
        
        // 加载字体
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            path.c_str(),
            fontConfig.SizePixels,
            &fontConfig,
            config.glyphRanges ? config.glyphRanges : getDefaultGlyphRanges()
        );
        
        if (!font) {
            return nullptr;
        }
        
        // 重建字体图集
        if (!rebuildFontAtlas()) {
            return nullptr;
        }
        
        // 创建字体资源
        auto fontResource = std::make_shared<FontResource>(path, font, config);
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
        return true;
    }
    return false;
}

std::shared_ptr<FontResource> FontManager::getDefaultFont() {
    return defaultFont_;
}

bool FontManager::rebuildFontAtlas() {
    try {
        ImGuiIO& io = ImGui::GetIO();
        return io.Fonts->Build();
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("rebuildFontAtlas(): " + std::string(e.what()));
        return false;
    }
}

void FontManager::unloadFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        // 如果是默认字体，清除默认字体引用
        if (defaultFont_ == it->second) {
            defaultFont_ = nullptr;
        }
        
        fonts_.erase(it);
        
        // 重建字体图集
        rebuildFontAtlas();
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

const ImWchar* FontManager::getDefaultGlyphRanges() {
    ImGuiIO& io = ImGui::GetIO();
    return io.Fonts->GetGlyphRangesDefault();
}

} // namespace Resource
} // namespace Core
} // namespace DearTs