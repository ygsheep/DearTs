/**
 * @file resource_manager.cpp
 * @brief 简化的资源管理系统实现
 * @author DearTs Team
 * @date 2024
 */

#include "resource_manager.h"
#include <SDL_image.h>
#include "../utils/logger.h"
#include "../utils/file_utils.h"
#include <iostream>

namespace DearTs {
namespace Core {
namespace Resource {

// 静态成员初始化
ResourceManager* ResourceManager::instance_ = nullptr;

/**
 * @brief 获取单例实例
 * @return ResourceManager实例指针
 */
ResourceManager* ResourceManager::getInstance() {
    if (!instance_) {
        DEARTS_LOG_DEBUG("Creating ResourceManager instance");
        instance_ = new ResourceManager();
    }
    return instance_;
}

/**
 * @brief 初始化资源管理器
 * @param renderer SDL渲染器
 * @return 是否成功
 */
bool ResourceManager::initialize(SDL_Renderer* renderer) {
    DEARTS_LOG_INFO("正在初始化资源管理器");
    
    if (!renderer) {
        DEARTS_LOG_ERROR("资源管理器: 无效的渲染器");
        return false;
    }
    
    renderer_ = renderer;
    
    // 初始化SDL_image
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        DEARTS_LOG_ERROR("资源管理器: 初始化SDL_image失败: " + std::string(IMG_GetError()));
        return false;
    }
    
    DEARTS_LOG_INFO("资源管理器初始化成功");
    return true;
}

/**
 * @brief 关闭资源管理器
 */
void ResourceManager::shutdown() {
    DEARTS_LOG_INFO("Shutting down ResourceManager");
    clearAll();
    
    IMG_Quit();
    renderer_ = nullptr;
    
    DEARTS_LOG_INFO("ResourceManager shutdown");
}

/**
 * @brief 加载纹理
 * @param path 纹理路径
 * @return 纹理资源指针
 */
std::shared_ptr<TextureResource> ResourceManager::loadTexture(const std::string& path) {
    DEARTS_LOG_DEBUG("Loading texture: " + path);
    
    // 检查是否已经加载
    auto it = resources_.find(path);
    if (it != resources_.end()) {
        auto texture_resource = std::dynamic_pointer_cast<TextureResource>(it->second);
        if (texture_resource) {
            DEARTS_LOG_DEBUG("Texture already loaded: " + path);
            return texture_resource;
        }
    }
    
    if (!renderer_) {
        DEARTS_LOG_ERROR("ResourceManager: Renderer not initialized");
        return nullptr;
    }
    
    // 检查文件是否存在
    if (!DearTs::Core::Utils::FileUtils::exists(path)) {
        DEARTS_LOG_ERROR("ResourceManager: File not found " + path);
        return nullptr;
    }
    
    // 加载图像表面
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        DEARTS_LOG_ERROR("ResourceManager: Failed to load image " + path + ": " + IMG_GetError());
        return nullptr;
    }
    
    DEARTS_LOG_DEBUG("Image loaded successfully: " + path + " (" + std::to_string(surface->w) + "x" + std::to_string(surface->h) + ")");
    
    // 创建纹理
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        DEARTS_LOG_ERROR("ResourceManager: Failed to create texture from " + path + ": " + SDL_GetError());
        return nullptr;
    }
    
    // 创建纹理资源
    auto texture_resource = std::make_shared<TextureResource>(path, texture);
    resources_[path] = texture_resource;
    
    DEARTS_LOG_INFO("ResourceManager: Loaded texture " + path);
    return texture_resource;
}

/**
 * @brief 获取纹理
 * @param path 纹理路径
 * @return 纹理资源指针
 */
std::shared_ptr<TextureResource> ResourceManager::getTexture(const std::string& path) {
    DEARTS_LOG_DEBUG("Getting texture: " + path);
    
    auto it = resources_.find(path);
    if (it != resources_.end()) {
        auto texture_resource = std::dynamic_pointer_cast<TextureResource>(it->second);
        if (texture_resource) {
            DEARTS_LOG_DEBUG("Texture found in cache: " + path);
            return texture_resource;
        }
    }
    
    // 如果没有找到，尝试加载
    DEARTS_LOG_DEBUG("Texture not found in cache, loading: " + path);
    return loadTexture(path);
}

/**
 * @brief 加载表面
 * @param path 表面路径
 * @return 表面资源指针
 */
std::shared_ptr<SurfaceResource> ResourceManager::loadSurface(const std::string& path) {
    DEARTS_LOG_DEBUG("加载表面: " + path);
    
    // 检查是否已经加载
    auto it = resources_.find(path);
    if (it != resources_.end()) {
        auto surface_resource = std::dynamic_pointer_cast<SurfaceResource>(it->second);
        if (surface_resource) {
            DEARTS_LOG_DEBUG("表面已加载: " + path);
            return surface_resource;
        }
    }
    
    // 检查文件是否存在
    if (!DearTs::Core::Utils::FileUtils::exists(path)) {
        DEARTS_LOG_ERROR("资源管理器: 文件未找到 " + path);
        return nullptr;
    }
    
    DEARTS_LOG_DEBUG("文件存在，尝试加载: " + path);
    
    // 加载图像表面
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        DEARTS_LOG_ERROR("资源管理器: 加载图像失败 " + path + ": " + IMG_GetError());
        return nullptr;
    }
    
    DEARTS_LOG_DEBUG("图像加载成功: " + path + " (" + std::to_string(surface->w) + "x" + std::to_string(surface->h) + ")");
    
    // 创建表面资源
    auto surface_resource = std::make_shared<SurfaceResource>(path, surface);
    resources_[path] = surface_resource;
    
    DEARTS_LOG_INFO("资源管理器: 已加载表面 " + path);
    return surface_resource;
}

/**
 * @brief 获取表面
 * @param path 表面路径
 * @return 表面资源指针
 */
std::shared_ptr<SurfaceResource> ResourceManager::getSurface(const std::string& path) {
    DEARTS_LOG_DEBUG("Getting surface: " + path);
    
    auto it = resources_.find(path);
    if (it != resources_.end()) {
        auto surface_resource = std::dynamic_pointer_cast<SurfaceResource>(it->second);
        if (surface_resource) {
            DEARTS_LOG_DEBUG("Surface found in cache: " + path);
            return surface_resource;
        }
    }
    
    // 如果没有找到，尝试加载
    DEARTS_LOG_DEBUG("Surface not found in cache, loading: " + path);
    return loadSurface(path);
}

/**
 * @brief 卸载资源
 * @param path 资源路径
 */
void ResourceManager::unloadResource(const std::string& path) {
    DEARTS_LOG_DEBUG("Unloading resource: " + path);
    
    auto it = resources_.find(path);
    if (it != resources_.end()) {
        resources_.erase(it);
        DEARTS_LOG_INFO("ResourceManager: Unloaded resource " + path);
    }
}

/**
 * @brief 清除所有资源
 */
void ResourceManager::clearAll() {
    DEARTS_LOG_DEBUG("Clearing all resources");
    
    resources_.clear();
    DEARTS_LOG_INFO("ResourceManager: Cleared all resources");
}

} // namespace Resource
} // namespace Core
} // namespace DearTs