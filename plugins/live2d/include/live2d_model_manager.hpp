/**
 * @file live2d_model_manager.hpp
 * @brief Live2D 模型管理器
 * @details 单例模式，负责模型的注册、加载、卸载、切换
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "live2d_model_instance.hpp"
#include "core/result.h"
#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace DearTs::Plugins::Live2D {

// 导入 Result 类型到当前命名空间
using DearTs::Core::Result;

/**
 * @brief 模型注册信息
 */
struct ModelRegistration {
    std::string model_id;           ///< 模型唯一 ID
    std::string model_name;         ///< 模型显示名称
    std::string model_directory;    ///< 模型所在目录
    std::string model_json_file;    ///< .model3.json 文件路径

    // 可选元数据
    std::string thumbnail_path;     ///< 缩略图路径（如果有）
    std::string author;             ///< 作者
    std::string version;            ///< 版本
};

/**
 * @brief Live2D 模型管理器
 *
 * @details
 * 单例模式，负责：
 * - 模型注册和发现
 * - 模型加载和卸载
 * - 活动模型切换
 * - 模型生命周期管理
 *
 * 使用示例：
 * ```cpp
 * auto& manager = Live2DModelManager::instance();
 *
 * // 注册模型
 * manager.register_model("haru", "Haru", "models/live2d/Haru/Haru.model3.json");
 *
 * // 加载模型
 * manager.load_model("haru");
 *
 * // 设置为活动模型
 * manager.set_active_model("haru");
 * ```
 */
class Live2DModelManager {
public:
    /**
     * @brief 获取单例实例
     */
    static Live2DModelManager& instance();

    /**
     * @brief 注册模型
     *
     * @param model_id 模型唯一 ID（不能重复）
     * @param registration 模型注册信息
     * @return Result<void, std::string> 成功或错误信息
     */
    Result<void, std::string> register_model(
        const std::string& model_id,
        const ModelRegistration& registration
    );

    /**
     * @brief 从目录扫描并注册模型
     *
     * @param directory 包含 .model3.json 的目录
     * @param recursive 是否递归扫描子目录
     * @return Result<size_t, std::string> 返回扫描到的模型数量
     */
    Result<size_t, std::string> scan_directory(
        const std::string& directory,
        bool recursive = true
    );

    /**
     * @brief 加载模型
     *
     * @param model_id 模型 ID
     * @return Result<void, std::string> 成功或错误信息
     */
    Result<void, std::string> load_model(const std::string& model_id);

    /**
     * @brief 卸载模型
     *
     * @param model_id 模型 ID
     */
    void unload_model(const std::string& model_id);

    /**
     * @brief 设置活动模型
     *
     * @param model_id 模型 ID（空字符串表示无活动模型）
     * @return Result<void, std::string> 成功或错误信息
     */
    Result<void, std::string> set_active_model(const std::string& model_id);

    /**
     * @brief 获取活动模型
     *
     * @return 活动模型指针，如果没有活动模型则返回 nullptr
     */
    Live2DModelInstance* get_active_model();

    /**
     * @brief 获取模型实例
     *
     * @param model_id 模型 ID
     * @return 模型实例指针，如果不存在则返回 nullptr
     */
    Live2DModelInstance* get_model(const std::string& model_id);

    /**
     * @brief 检查模型是否已加载
     *
     * @param model_id 模型 ID
     * @return true 如果模型已加载
     */
    bool is_model_loaded(const std::string& model_id) const;

    /**
     * @brief 获取所有已注册的模型 ID
     *
     * @return 模型 ID 列表
     */
    std::vector<std::string> get_registered_models() const;

    /**
     * @brief 获取所有已加载的模型 ID
     *
     * @return 模型 ID 列表
     */
    std::vector<std::string> get_loaded_models() const;

    /**
     * @brief 获取模型注册信息
     *
     * @param model_id 模型 ID
     * @return 模型注册信息指针，如果不存在则返回 nullptr
     */
    const ModelRegistration* get_model_info(const std::string& model_id) const;

    /**
     * @brief 获取活动模型 ID
     *
     * @return 活动模型 ID，如果没有活动模型则返回空字符串
     */
    const std::string& get_active_model_id() const { return m_active_model_id; }

    /**
     * @brief 卸载所有模型
     */
    void unload_all_models();

    /**
     * @brief 清除所有注册信息
     */
    void clear_all_registrations();

private:
    Live2DModelManager() = default;
    ~Live2DModelManager();

    // 禁止拷贝和移动
    Live2DModelManager(const Live2DModelManager&) = delete;
    Live2DModelManager& operator=(const Live2DModelManager&) = delete;
    Live2DModelManager(Live2DModelManager&&) = delete;
    Live2DModelManager& operator=(Live2DModelManager&&) = delete;

    /**
     * @brief 查找目录中的 .model3.json 文件
     *
     * @param directory 目录路径
     * @param recursive 是否递归
     * @return 找到的 .model3.json 文件列表
     */
    std::vector<std::string> find_model_json_files(
        const std::string& directory,
        bool recursive
    ) const;

    /**
     * @brief 从 .model3.json 提取模型 ID
     *
     * @param json_path .model3.json 文件路径
     * @return 提取的模型 ID
     */
    std::string extract_model_id(const std::string& json_path) const;

private:
    // 已注册的模型信息
    std::unordered_map<std::string, ModelRegistration> m_registered_models;

    // 已加载的模型实例
    std::unordered_map<std::string, std::unique_ptr<Live2DModelInstance>> m_loaded_models;

    // 活动模型 ID
    std::string m_active_model_id;
};

} // namespace DearTs::Plugins::Live2D
