/**
 * @file live2d_model_instance.cpp
 * @brief Live2D 模型实例实现
 */

#include "live2d_model_instance.hpp"
#include "CubismUserModel.hpp"              // Live2D SDK
#include "CubismModelSettingJson.hpp"       // Live2D SDK
#include "CubismFramework.hpp"              // Live2D SDK
#include "Id/CubismIdManager.hpp"           // Live2D SDK
#include "liblogger/logger.h"
#include <GL/glew.h>

namespace DearTs::Plugins::Live2D {

// ============================================================================
// 构造函数和析构函数
// ============================================================================

Live2DModelInstance::Live2DModelInstance() = default;

Live2DModelInstance::~Live2DModelInstance() {
    unload();
}

// ============================================================================
// 模型加载
// ============================================================================

Result<void, std::string> Live2DModelInstance::load_from_directory(
    const std::filesystem::path& model_dir
) {
    // 查找 model.json
    std::filesystem::path model_json_path = model_dir / "model.json";

    if (!std::filesystem::exists(model_json_path)) {
        std::string error_msg = "model.json not found in: ";
        error_msg += model_dir.string();
        return Result<void, std::string>::err(error_msg);
    }

    return load_from_json(model_json_path);
}

Result<void, std::string> Live2DModelInstance::load_from_json(
    const std::filesystem::path& model_json_path
) {
    if (m_load_state == ModelLoadState::Loading) {
        return Result<void, std::string>::err("Model is already loading");
    }

    if (m_load_state == ModelLoadState::Loaded) {
        unload();
    }

    m_load_state = ModelLoadState::Loading;
    m_last_error.clear();

    LOG_INFO("Live2DModelInstance: Loading model from: {}",
             model_json_path.string());

    try {
        // TODO: 实现 Live2D 模型加载
        // 1. 创建 CubismUserModel
        // 2. 加载 model.json
        // 3. 加载纹理
        // 4. 加载动作、物理、表情等

        // 伪代码示例：
        // m_cubism_model = std::make_unique<CubismUserModel>();
        // m_model_setting = CubismModelSettingJson::Load(model_json_path);
        // load_textures();
        // load_motions();
        // load_physics();
        // load_expressions();

        // 设置模型信息
        setup_model_info();

        m_load_state = ModelLoadState::Loaded;
        LOG_INFO("Live2DModelInstance: Model loaded successfully: {}",
                 m_info.model_name);

        return Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        m_load_state = ModelLoadState::Error;
        m_last_error = e.what();
        LOG_ERROR("Live2DModelInstance: Failed to load model: {}", e.what());
        return Result<void, std::string>::err(e.what());
    }
}

void Live2DModelInstance::unload() {
    if (m_load_state == ModelLoadState::Unloaded) {
        return;
    }

    LOG_INFO("Live2DModelInstance: Unloading model: {}", m_info.model_name);

    // TODO: 清理 Live2D 模型资源
    // - 释放纹理
    // - 释放动作
    // - 释放物理
    // - 删除模型对象

    m_cubism_model.reset();
    m_model_setting.reset();
    m_info = ModelInfo{};
    m_load_state = ModelLoadState::Unloaded;

    LOG_INFO("Live2DModelInstance: Model unloaded");
}

// ============================================================================
// 更新和渲染
// ============================================================================

void Live2DModelInstance::update(float delta_time) {
    if (!is_loaded() || !m_cubism_model) {
        return;
    }

    // TODO: 更新模型
    // m_cubism_model->Update();
}

void Live2DModelInstance::draw() {
    if (!is_loaded() || !m_cubism_model) {
        return;
    }

    // TODO: 渲染模型
    // m_cubism_model->Draw();
}

bool Live2DModelInstance::is_valid() const {
    return is_loaded() && m_cubism_model != nullptr;
}

// ============================================================================
// 参数控制
// ============================================================================

void Live2DModelInstance::set_parameter(const std::string& parameter_name, float value) {
    if (!m_cubism_model) {
        return;
    }

    // TODO: 设置参数
    // Live2D::Cubism::Framework::CubismId* id =
    //     Live2D::Cubism::Framework::CubismIdManager::GetId(parameter_name.c_str());
    // m_cubism_model->SetParameterValue(id, value);
}

float Live2DModelInstance::get_parameter(const std::string& parameter_name) const {
    if (!m_cubism_model) {
        return 0.0f;
    }

    // TODO: 获取参数
    // Live2D::Cubism::Framework::CubismId* id =
    //     Live2D::Cubism::Framework::CubismIdManager::GetId(parameter_name.c_str());
    // return m_cubism_model->GetParameterValue(id);

    return 0.0f;
}

void Live2DModelInstance::add_parameter(const std::string& parameter_name, float value) {
    if (!m_cubism_model) {
        return;
    }

    // TODO: 添加参数值
    // Live2D::Cubism::Framework::CubismId* id =
    //     Live2D::Cubism::Framework::CubismIdManager::GetId(parameter_name.c_str());
    // m_cubism_model->AddParameterValue(id, value);
}

// ============================================================================
// 部位透明度
// ============================================================================

void Live2DModelInstance::set_part_opacity(int part_index, float opacity) {
    if (!m_cubism_model) {
        return;
    }

    // TODO: 设置部位透明度
    // m_cubism_model->SetPartOpacity(part_index, opacity);
}

float Live2DModelInstance::get_part_opacity(int part_index) const {
    if (!m_cubism_model) {
        return 0.0f;
    }

    // TODO: 获取部位透明度
    // return m_cubism_model->GetPartOpacity(part_index);

    return 0.0f;
}

// ============================================================================
// 私有方法
// ============================================================================

Result<void, std::string> Live2DModelInstance::load_texture(
    const std::filesystem::path& texture_path,
    int texture_index
) {
    LOG_INFO("Live2DModelInstance: Loading texture {}: {}", texture_index, texture_path.string());

    // TODO: 加载纹理
    // 1. 使用 SDL_image 加载图像
    // 2. 创建 OpenGL 纹理
    // 3. 绑定到 Live2D 模型

    return Result<void, std::string>::ok();
}

void Live2DModelInstance::load_motions() {
    LOG_DEBUG("Live2DModelInstance: Loading motions");
    // TODO: 加载动作文件
}

void Live2DModelInstance::load_physics() {
    LOG_DEBUG("Live2DModelInstance: Loading physics");
    // TODO: 加载物理文件
}

void Live2DModelInstance::load_expressions() {
    LOG_DEBUG("Live2DModelInstance: Loading expressions");
    // TODO: 加载表情文件
}

void Live2DModelInstance::setup_model_info() {
    if (!m_cubism_model) {
        return;
    }

    // TODO: 从 model.json 读取模型信息
    m_info.model_name = "Live2D Model";
    m_info.width = 512;
    m_info.height = 512;
    m_info.has_motion = false;
    m_info.has_physics = false;
    m_info.has_eye_blink = false;
    m_info.has_breath = false;
}

} // namespace DearTs::Plugins::Live2D
