/**
 * @file font_resource.h
 * @brief 字体资源管理
 * @details 提供ImGui字体加载和管理功能，支持中文字体和图标字体
 * @author DearTs Team
 * @date 2025
 */

#pragma once

#include "resource_manager.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

namespace DearTs {
namespace Core {
namespace Resource {

/**
 * @brief 字体配置结构体
 */
struct FontConfig {
    std::string name;           ///< 字体名称
    std::string path;           ///< 字体文件路径
    float size;                 ///< 字体大小
    float scale;                ///< 缩放因子
    const ImWchar* glyphRanges; ///< 字符范围
    bool mergeMode;             ///< 是否为合并模式
    
    /**
     * @brief 构造函数
     */
    FontConfig(const std::string& name = "",
               const std::string& path = "",
               float size = 11.0f,
               float scale = 1.0f,
               const ImWchar* glyphRanges = nullptr,
               bool mergeMode = false)
        : name(name), path(path), size(size), scale(scale),
          glyphRanges(glyphRanges), mergeMode(mergeMode) {}
};

/**
 * @brief 字体资源类
 */
class FontResource : public Resource {
public:
    /**
     * @brief 构造函数
     * @param path 字体路径
     * @param font ImGui字体指针
     * @param config 字体配置
     */
    FontResource(const std::string& path, ImFont* font, const FontConfig& config)
        : Resource(path, ResourceType::FONT), font_(font), config_(config) {}
    
    /**
     * @brief 析构函数
     */
    ~FontResource() = default;
    
    /**
     * @brief 获取ImGui字体
     * @return ImGui字体指针
     */
    ImFont* getFont() const { return font_; }
    
    /**
     * @brief 获取字体配置
     * @return 字体配置
     */
    const FontConfig& getConfig() const { return config_; }
    
    /**
     * @brief 设置为当前字体
     */
    void pushFont() const {
        if (font_) {
            ImGui::PushFont(font_);
        }
    }
    
    /**
     * @brief 恢复之前的字体
     */
    void popFont() const {
        if (font_) {
            ImGui::PopFont();
        }
    }
    
private:
    ImFont* font_;          ///< ImGui字体指针
    FontConfig config_;     ///< 字体配置
};

/**
 * @brief 字体管理器类
 * @details 提供字体加载、管理和使用功能
 */
class FontManager {
public:
    /**
     * @brief 获取单例实例
     * @return FontManager实例指针
     */
    static FontManager* getInstance();
    
    /**
     * @brief 初始化字体管理器
     * @return 是否初始化成功
     */
    bool initialize();
    
    /**
     * @brief 关闭字体管理器
     */
    void shutdown();
    
    /**
     * @brief 加载默认字体（包含中文支持）
     * @param fontSize 字体大小
     * @param scaleFactor 缩放因子
     * @return 是否加载成功
     */
    bool loadDefaultFont(float fontSize = 11.0f, float scaleFactor = 1.0f);
    
    /**
     * @brief 从文件加载字体
     * @param name 字体名称
     * @param path 字体文件路径
     * @param config 字体配置
     * @return 字体资源指针
     */
    std::shared_ptr<FontResource> loadFontFromFile(const std::string& name,
                                                   const std::string& path,
                                                   const FontConfig& config);
    
    /**
     * @brief 加载图标字体
     * @param name 字体名称
     * @param path 字体文件路径
     * @param fontSize 字体大小
     * @param iconRanges 图标字符范围
     * @return 字体资源指针
     */
    std::shared_ptr<FontResource> loadIconFont(const std::string& name,
                                               const std::string& path,
                                               float fontSize,
                                               const ImWchar* iconRanges);
    
    /**
     * @brief 获取字体资源
     * @param name 字体名称
     * @return 字体资源指针
     */
    std::shared_ptr<FontResource> getFont(const std::string& name);
    
    /**
     * @brief 设置默认字体
     * @param name 字体名称
     * @return 是否设置成功
     */
    bool setDefaultFont(const std::string& name);
    
    /**
     * @brief 获取默认字体
     * @return 默认字体资源指针
     */
    std::shared_ptr<FontResource> getDefaultFont();
    
    /**
     * @brief 重建字体图集
     * @return 是否重建成功
     */
    bool rebuildFontAtlas();
    
    /**
     * @brief 卸载字体
     * @param name 字体名称
     */
    void unloadFont(const std::string& name);
    
    /**
     * @brief 清除所有字体
     */
    void clearAll();
    
    /**
     * @brief 获取中文字符范围
     * @return 中文字符范围
     */
    static const ImWchar* getChineseGlyphRanges();
    
    /**
     * @brief 获取默认字符范围
     * @return 默认字符范围
     */
    static const ImWchar* getDefaultGlyphRanges();
    
    /**
     * @brief 字体作用域辅助类
     * @details RAII方式管理字体的push/pop
     */
    class FontScope {
    public:
        /**
         * @brief 构造函数
         * @param fontResource 字体资源
         */
        explicit FontScope(std::shared_ptr<FontResource> fontResource)
            : fontResource_(fontResource) {
            if (fontResource_) {
                fontResource_->pushFont();
            }
        }
        
        /**
         * @brief 析构函数
         */
        ~FontScope() {
            if (fontResource_) {
                fontResource_->popFont();
            }
        }
        
        // 禁止拷贝和移动
        FontScope(const FontScope&) = delete;
        FontScope& operator=(const FontScope&) = delete;
        FontScope(FontScope&&) = delete;
        FontScope& operator=(FontScope&&) = delete;
        
    private:
        std::shared_ptr<FontResource> fontResource_;
    };
    
private:
    /**
     * @brief 私有构造函数
     */
    FontManager() = default;
    
    /**
     * @brief 私有析构函数
     */
    ~FontManager() = default;
    
    static FontManager* instance_;                                              ///< 单例实例
    std::unordered_map<std::string, std::shared_ptr<FontResource>> fonts_;     ///< 字体资源映射
    std::shared_ptr<FontResource> defaultFont_;                                 ///< 默认字体
    float currentScale_ = 1.0f;                                                ///< 当前缩放因子
    bool initialized_ = false;                                                  ///< 是否已初始化
};

} // namespace Resource
} // namespace Core
} // namespace DearTs

// 便利宏定义
#define FONT_MANAGER DearTs::Core::Resource::FontManager::getInstance()
#define FONT_SCOPE(fontName) DearTs::Core::Resource::FontManager::FontScope _fontScope(FONT_MANAGER->getFont(fontName))

