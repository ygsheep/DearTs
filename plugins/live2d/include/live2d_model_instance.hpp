/**
 * @file live2d_model_instance.hpp
 * @brief Live2D 模型实例
 * @details 封装 Live2D Cubism 模型的加载和渲染
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <filesystem>
#include <memory>

namespace Live2D { namespace Cubism { namespace Framework {
    class CubismUserModel;
    class CubismModelSettingJson;
}}}

namespace DearTs::Plugins::Live2D {

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
 * 封装 Live2D Cubism SDK 的模型对象。
 *
 * 关键功能：
 * - 从 model.json 加载模型
 * - 管理模型生命周期
 * - 更新和渲染模型
 * - 参数控制
 *
 * @note
 * - 模型目录结构：
 *   - model.json (模型配置)
 *   - *.moc3 (模型数据)
 *   - *.png (纹理)
 *   - *.motion3.json (动作)
 *   - *.physics3.json (物理)
 *   - *.exp3.json (表情)
 * - 线程安全：单线程使用
 */
class Live2DModelInstance {
public:
    /**
     * @brief 构造函数
     */
    Live2DModelInstance();

    /**
     * @brief 析构函数
     */
    ~Live2DModelInstance();

    // 禁止拷贝和移动
    Live2DModelInstance(const Live2DModelInstance&) = delete;
    Live2DModelInstance& operator=(const Live2DModelInstance&) = delete;
    Live2DModelInstance(Live2DModelInstance&&) = delete;
    Live2DModelInstance& operator=(Live2DModelInstance&&) = delete;

    /**
     * @brief 从目录加载模型
     * @param model_dir 模型目录（包含 model.json）
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_directory(const std::filesystem::path& model_dir);

    /**
     * @brief 从 model.json 加载模型
     * @param model_json_path model.json 文件路径
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_json(const std::filesystem::path& model_json_path);

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
    [[nodiscard]] const ModelInfo& get_info() const { return m_info; }

    /**
     * @brief 获取加载状态
     */
    [[nodiscard]] ModelLoadState get_load_state() const { return m_load_state; }

    /**
     * @brief 检查模型是否已加载
     */
    [[nodiscard]] bool is_loaded() const { return m_load_state == ModelLoadState::Loaded; }

    /**
     * @brief 检查模型是否有效
     */
    [[nodiscard]] bool is_valid() const;

    /**
     * @brief 获取原始 Cubism 模型对象（内部使用）
     */
    [[nodiscard]] Live2D::Cubism::Framework::CubismUserModel* get_cubism_model() const {
        return m_cubism_model.get();
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
    [[nodiscard]] float get_parameter(const std::string& parameter_name) const;

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
    [[nodiscard]] float get_part_opacity(int part_index) const;

private:
    /**
     * @brief 加载模型纹理
     * @param texture_path 纹理路径
     * @param texture_index 纹理索引
     */
    Result<void, std::string> load_texture(
        const std::filesystem::path& texture_path,
        int texture_index
    );

    /**
     * @brief 加载动作文件
     */
    void load_motions();

    /**
     * @brief 加载物理文件
     */
    void load_physics();

    /**
     * @brief 加载表情文件
     */
    void load_expressions();

    /**
     * @brief 设置模型信息
     */
    void setup_model_info();

private:
    std::unique_ptr<Live2D::Cubism::Framework::CubismUserModel> m_cubism_model;
    std::unique_ptr<Live2D::Cubism::Framework::CubismModelSettingJson> m_model_setting;
    ModelLoadState m_load_state = ModelLoadState::Unloaded;
    ModelInfo m_info;
    std::string m_last_error;
};

} // namespace DearTs::Plugins::Live2D
