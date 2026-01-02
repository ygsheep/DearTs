/**
 * @file live2d_model_instance.hpp
 * @brief Live2D 模型实例
 * @details 封装 Live2D Cubism 模型的加载和渲染
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

// 先包含 DearTs 核心头文件，避免宏污染
#include "core/result.h"
#include <string>
#include <filesystem>
#include <memory>

// 再包含 Live2D Framework 头文件
#include <CubismFramework.hpp>
#include <Model/CubismUserModel.hpp>
#include <ICubismModelSetting.hpp>

// Live2D Framework 命名空间别名
namespace L2D = Live2D::Cubism::Framework;

namespace DearTs::Plugins::Live2D {

// 导入 Result 类型到当前命名空间
using DearTs::Core::Result;

/**
 * @brief 模型加载状态
 */
enum class ModelLoadState {
    Unloaded,       ///< 未加载
    Loading,        ///< 加载中
    Loaded,         ///< 已加载
    Error           ///< 错误
};

/**
 * @brief 模型信息
 */
struct ModelInfo {
    std::string model_name;          ///< 模型名称
    std::string model_path;          ///< 模型目录路径
    int model_version = 0;           ///< 模型版本
    int width = 0;                   ///< 模型宽度
    int height = 0;                  ///< 模型高度
    bool has_motion = false;         ///< 是否有动作
    bool has_physics = false;        ///< 是否有物理
    bool has_eye_blink = false;      ///< 是否有眨眼
    bool has_breath = false;         ///< 是否有呼吸
};

/**
 * @brief Live2D 模型实例
 *
 * @details
 * 封装 Live2D Cubism SDK 的模型对象，继承自 CubismUserModel。
 *
 * 关键功能：
 * - 从 .model3.json 加载模型（Cubism SDK 4.x 格式）
 * - 管理模型生命周期
 * - 更新和渲染模型
 * - 参数控制
 *
 * @note
 * - 模型目录结构：
 *   - *.model3.json (模型配置，Cubism SDK 4.x 格式)
 *   - *.moc3 (模型数据)
 *   - *.png (纹理)
 *   - *.motion3.json (动作)
 *   - *.physics3.json (物理)
 *   - *.exp3.json (表情)
 * - 线程安全：单线程使用
 */
class Live2DModelInstance : public L2D::CubismUserModel {
public:
    /**
     * @brief 构造函数
     */
    Live2DModelInstance();

    /**
     * @brief 析构函数
     */
    ~Live2DModelInstance() override;

    // 禁止拷贝和移动
    Live2DModelInstance(const Live2DModelInstance&) = delete;
    Live2DModelInstance& operator=(const Live2DModelInstance&) = delete;
    Live2DModelInstance(Live2DModelInstance&&) = delete;
    Live2DModelInstance& operator=(Live2DModelInstance&&) = delete;

    /**
     * @brief 从目录加载模型
     * @param model_dir 模型目录（包含 .model3.json 文件）
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_directory(const std::filesystem::path& model_dir);

    /**
     * @brief 从 .model3.json 加载模型
     * @param model3_json_path .model3.json 文件路径
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_model3_json(const std::filesystem::path& model3_json_path);

    /**
     * @brief 卸载模型
     */
    void unload();

    /**
     * @brief 更新模型状态
     * @param delta_time 时间增量（秒）
     */
    void update(float delta_time);

    /**
     * @brief 渲染模型
     */
    void draw();

    /**
     * @brief 获取模型信息
     */
    const ModelInfo& get_info() const { return m_info; }

    /**
     * @brief 获取加载状态
     */
    ModelLoadState get_load_state() const { return m_load_state; }

    /**
     * @brief 检查模型是否已加载
     */
    bool is_loaded() const { return m_load_state == ModelLoadState::Loaded; }

    /**
     * @brief 检查模型是否有效
     */
    bool is_valid() const;

    /**
     * @brief 获取原始 Cubism 模型对象（内部使用）
     */
    L2D::CubismModel* get_cubism_model() const {
        return GetModel();
    }

    /**
     * @brief 获取模型设置（内部使用）
     */
    L2D::ICubismModelSetting* get_model_setting() const {
        return _modelSetting;
    }

    /**
     * @brief 获取模型目录（内部使用）
     */
    const std::filesystem::path& get_model_directory() const {
        return m_model_directory;
    }

    // ========================================================================
    // 参数控制
    // ========================================================================

    /**
     * @brief 设置模型参数值
     * @param parameter_name 参数名称
     * @param value 参数值
     */
    void set_parameter(const std::string& parameter_name, float value);

    /**
     * @brief 获取模型参数值
     * @param parameter_name 参数名称
     * @return 参数值，失败返回 0.0
     */
    float get_parameter(const std::string& parameter_name) const;

    /**
     * @brief 添加模型参数值
     * @param parameter_name 参数名称
     * @param value 增量
     */
    void add_parameter(const std::string& parameter_name, float value);

    // ========================================================================
    // 部位透明度
    // ========================================================================

    /**
     * @brief 设置部位透明度
     * @param part_index 部位索引
     * @param opacity 透明度 (0.0 - 1.0)
     */
    void set_part_opacity(int part_index, float opacity);

    /**
     * @brief 获取部位透明度
     * @param part_index 部位索引
     * @return 透明度
     */
    float get_part_opacity(int part_index) const;

protected:
    /**
     * @brief 创建文件缓冲区（CubismUserModel 虚方法）
     * @param path 文件路径
     * @param size 输出文件大小
     * @return 文件缓冲区，失败返回 nullptr
     */
    L2D::csmByte* CreateBuffer(const L2D::csmChar* path, L2D::csmSizeInt* size);

    /**
     * @brief 释放文件缓冲区（CubismUserModel 虚方法）
     * @param buffer 缓冲区
     * @param path 文件路径（可选）
     */
    void DeleteBuffer(L2D::csmByte* buffer, const L2D::csmChar* path = "");

private:
    /**
     * @brief 从 ICubismModelSetting 设置模型
     * @param setting 模型设置
     */
    Result<void, std::string> setup_model(L2D::ICubismModelSetting* setting);

    /**
     * @brief 加载模型纹理
     */
    Result<void, std::string> setup_textures();

    /**
     * @brief 预加载动作组
     * @param group 动作组名称
     */
    void preload_motion_group(const L2D::csmChar* group);

    /**
     * @brief 设置模型信息
     */
    void setup_model_info();

private:
    // 模型设置
    L2D::ICubismModelSetting* _modelSetting = nullptr;
    std::filesystem::path m_model_directory;

    // 模型加载状态
    ModelLoadState m_load_state = ModelLoadState::Unloaded;
    ModelInfo m_info;
    std::string m_last_error;
};

} // namespace DearTs::Plugins::Live2D
