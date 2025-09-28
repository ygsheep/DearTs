/**
 * @file resource_manager.h
 * @brief 简化的资源管理系统
 * @author DearTs Team
 * @date 2024
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <SDL.h>
// Logger removed - using simple output instead

namespace DearTs {
namespace Core {
namespace Resource {

/**
 * @brief 资源类型枚举
 */
enum class ResourceType {
    TEXTURE,
    SURFACE,  // 新增表面资源类型
    SOUND,
    FONT,
    UNKNOWN
};

/**
 * @brief 基础资源类
 */
class Resource {
public:
    /**
     * @brief 构造函数
     * @param path 资源路径
     * @param type 资源类型
     */
    Resource(const std::string& path, ResourceType type)
        : path_(path), type_(type) {}
    
    /**
     * @brief 虚析构函数
     */
    virtual ~Resource() = default;
    
    /**
     * @brief 获取资源路径
     * @return 资源路径
     */
    const std::string& getPath() const { return path_; }
    
    /**
     * @brief 获取资源类型
     * @return 资源类型
     */
    ResourceType getType() const { return type_; }
    
protected:
    std::string path_;
    ResourceType type_;
};

/**
 * @brief 纹理资源类
 */
class TextureResource : public Resource {
public:
    /**
     * @brief 构造函数
     * @param path 纹理路径
     * @param texture SDL纹理指针
     */
    TextureResource(const std::string& path, SDL_Texture* texture)
        : Resource(path, ResourceType::TEXTURE), texture_(texture) {}
    
    /**
     * @brief 析构函数
     */
    ~TextureResource() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
    }
    
    /**
     * @brief 获取SDL纹理
     * @return SDL纹理指针
     */
    SDL_Texture* getTexture() const { return texture_; }
    
private:
    SDL_Texture* texture_;
};

/**
 * @brief 表面资源类
 */
class SurfaceResource : public Resource {
public:
    /**
     * @brief 构造函数
     * @param path 表面路径
     * @param surface SDL表面指针
     */
    SurfaceResource(const std::string& path, SDL_Surface* surface)
        : Resource(path, ResourceType::SURFACE), surface_(surface) {}
    
    /**
     * @brief 析构函数
     */
    ~SurfaceResource() {
        if (surface_) {
            SDL_FreeSurface(surface_);
        }
    }
    
    /**
     * @brief 获取SDL表面
     * @return SDL表面指针
     */
    SDL_Surface* getSurface() const { return surface_; }
    
private:
    SDL_Surface* surface_;
};

/**
 */
class ResourceManager {
public:
    /**
     * @brief 获取单例实例
     * @return ResourceManager实例指针
     */
    static ResourceManager* getInstance();
    
    /**
     * @brief 初始化资源管理器
     * @param renderer SDL渲染器
     * @return 是否成功
     */
    bool initialize(SDL_Renderer* renderer);
    
    /**
     * @brief 关闭资源管理器
     */
    void shutdown();
    
    /**
     * @brief 加载纹理
     * @param path 纹理路径
     * @return 纹理资源指针
     */
    std::shared_ptr<TextureResource> loadTexture(const std::string& path);
    
    /**
     * @brief 获取纹理
     * @param path 纹理路径
     * @return 纹理资源指针
     */
    std::shared_ptr<TextureResource> getTexture(const std::string& path);
    
    /**
     * @brief 加载表面
     * @param path 表面路径
     * @return 表面资源指针
     */
    std::shared_ptr<SurfaceResource> loadSurface(const std::string& path);
    
    /**
     * @brief 获取表面
     * @param path 表面路径
     * @return 表面资源指针
     */
    std::shared_ptr<SurfaceResource> getSurface(const std::string& path);
    
    /**
     * @brief 卸载资源
     * @param path 资源路径
     */
    void unloadResource(const std::string& path);
    
    /**
     * @brief 清除所有资源
     */
    void clearAll();
    
private:
    /**
     * @brief 私有构造函数
     */
    ResourceManager() = default;
    
    /**
     * @brief 私有析构函数
     */
    ~ResourceManager() = default;
    
    static ResourceManager* instance_;
    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Resource>> resources_;
};

} // namespace Resource
} // namespace Core
} // namespace DearTs

#define RESOURCE_MANAGER DearTs::Core::Resource::ResourceManager::getInstance()

