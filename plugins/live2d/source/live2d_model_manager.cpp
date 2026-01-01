/**
 * @file live2d_model_manager.cpp
 * @brief Live2D 模型管理器实现
 */

#include "live2d_model_manager.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <filesystem>

#ifdef ERROR
#undef ERROR
#endif

namespace DearTs::Plugins::Live2D {

namespace fs = std::filesystem;

// ============================================================================
// 单例实现
// ============================================================================

Live2DModelManager& Live2DModelManager::instance() {
    static Live2DModelManager s_instance;
    return s_instance;
}

Live2DModelManager::~Live2DModelManager() {
    LOG_INFO("Live2DModelManager: Destroying manager, unloading all models");
    unload_all_models();
}

// ============================================================================
// 模型注册
// ============================================================================

Result<void, std::string> Live2DModelManager::register_model(
    const std::string& model_id,
    const ModelRegistration& registration
) {
    // 检查 ID 是否已存在
    if (m_registered_models.find(model_id) != m_registered_models.end()) {
        return Result<void, std::string>::err("Model ID already registered: " + model_id);
    }

    // 验证 model3.json 文件是否存在
    if (!fs::exists(registration.model_json_file)) {
        return Result<void, std::string>::err(
            "Model JSON file not found: " + registration.model_json_file
        );
    }

    // 注册模型
    m_registered_models[model_id] = registration;

    LOG_INFO("Live2DModelManager: Registered model '{}' from '{}'",
             model_id, registration.model_directory);

    return Result<void, std::string>::ok();
}

Result<size_t> Live2DModelManager::scan_directory(
    const std::string& directory,
    bool recursive
) {
    if (!fs::exists(directory)) {
        return Result<size_t, std::string>::err("Directory does not exist: " + directory);
    }

    LOG_INFO("Live2DModelManager: Scanning directory '{}'", directory);

    // 查找所有 .model3.json 文件
    auto model_files = find_model_json_files(directory, recursive);

    if (model_files.empty()) {
        LOG_WARN("Live2DModelManager: No .model3.json files found in '{}'", directory);
        return Result<size_t, std::string>::ok(0);
    }

    // 注册所有找到的模型
    size_t registered_count = 0;
    for (const auto& json_file : model_files) {
        try {
            // 读取 JSON 文件获取模型名称
            std::ifstream file(json_file);
            nlohmann::json json;
            file >> json;

            // 提取模型 ID
            std::string model_id = extract_model_id(json_file);

            // 提取模型目录
            std::string model_dir = fs::path(json_file).parent_path().string();

            // 构造注册信息
            ModelRegistration registration;
            registration.model_id = model_id;
            registration.model_name = model_id; // 默认使用 ID 作为名称
            registration.model_directory = model_dir;
            registration.model_json_file = json_file;

            // 检查是否有缩略图
            fs::path icon_path = fs::path(model_dir) / "icon.png";
            if (fs::exists(icon_path)) {
                registration.thumbnail_path = icon_path.string();
            }

            // 注册模型
            auto result = register_model(model_id, registration);
            if (result.isErr()) {
                LOG_WARN("Live2DModelManager: Failed to register '{}': {}",
                         model_id, result.error());
            } else {
                registered_count++;
            }

        } catch (const std::exception& e) {
            LOG_ERROR("Live2DModelManager: Failed to parse '{}': {}", json_file, e.what());
        }
    }

    LOG_INFO("Live2DModelManager: Scanned and registered {} models", registered_count);
    return Result<size_t, std::string>::ok(registered_count);
}

// ============================================================================
// 模型加载和卸载
// ============================================================================

Result<void, std::string> Live2DModelManager::load_model(const std::string& model_id) {
    // 检查模型是否已注册
    auto reg_it = m_registered_models.find(model_id);
    if (reg_it == m_registered_models.end()) {
        return Result<void, std::string>::err("Model not registered: " + model_id);
    }

    // 检查模型是否已加载
    if (m_loaded_models.find(model_id) != m_loaded_models.end()) {
        LOG_WARN("Live2DModelManager: Model '{}' already loaded", model_id);
        return Result<void, std::string>::ok();
    }

    LOG_INFO("Live2DModelManager: Loading model '{}'", model_id);

    // 创建模型实例
    auto model_instance = std::make_unique<Live2DModelInstance>();

    // 从 model3.json 文件所在的目录加载模型
    std::filesystem::path model_dir = fs::path(reg_it->second.model_json_file).parent_path();

    // 加载模型
    auto result = model_instance->load_from_directory(model_dir);
    if (result.isErr()) {
        return Result<void, std::string>::err(
            "Failed to load model '" + model_id + "': " + result.error()
        );
    }

    // 添加到已加载列表
    m_loaded_models[model_id] = std::move(model_instance);

    LOG_INFO("Live2DModelManager: Model '{}' loaded successfully", model_id);
    return Result<void, std::string>::ok();
}

void Live2DModelManager::unload_model(const std::string& model_id) {
    auto it = m_loaded_models.find(model_id);
    if (it == m_loaded_models.end()) {
        return;
    }

    LOG_INFO("Live2DModelManager: Unloading model '{}'", model_id);

    // 如果是活动模型，清除活动状态
    if (m_active_model_id == model_id) {
        m_active_model_id.clear();
    }

    // 卸载模型
    it->second->unload();
    m_loaded_models.erase(it);

    LOG_INFO("Live2DModelManager: Model '{}' unloaded", model_id);
}

void Live2DModelManager::unload_all_models() {
    LOG_INFO("Live2DModelManager: Unloading all models");

    // 清除活动模型
    m_active_model_id.clear();

    // 卸载所有模型
    m_loaded_models.clear();

    LOG_INFO("Live2DModelManager: All models unloaded");
}

// ============================================================================
// 活动模型管理
// ============================================================================

Result<void, std::string> Live2DModelManager::set_active_model(const std::string& model_id) {
    // 空字符串表示取消活动模型
    if (model_id.empty()) {
        m_active_model_id.clear();
        LOG_INFO("Live2DModelManager: Active model cleared");
        return Result<void, std::string>::ok();
    }

    // 检查模型是否已加载
    if (m_loaded_models.find(model_id) == m_loaded_models.end()) {
        return Result<void, std::string>::err(
            "Cannot set active model: '" + model_id + "' is not loaded"
        );
    }

    // 设置活动模型
    m_active_model_id = model_id;

    LOG_INFO("Live2DModelManager: Active model set to '{}'", model_id);
    return Result<void, std::string>::ok();
}

Live2DModelInstance* Live2DModelManager::get_active_model() {
    if (m_active_model_id.empty()) {
        return nullptr;
    }

    auto it = m_loaded_models.find(m_active_model_id);
    if (it == m_loaded_models.end()) {
        LOG_WARN("Live2DModelManager: Active model '{}' not found in loaded models",
                 m_active_model_id);
        m_active_model_id.clear();
        return nullptr;
    }

    return it->second.get();
}

Live2DModelInstance* Live2DModelManager::get_model(const std::string& model_id) {
    auto it = m_loaded_models.find(model_id);
    if (it == m_loaded_models.end()) {
        return nullptr;
    }
    return it->second.get();
}

bool Live2DModelManager::is_model_loaded(const std::string& model_id) const {
    return m_loaded_models.find(model_id) != m_loaded_models.end();
}

// ============================================================================
// 查询方法
// ============================================================================

std::vector<std::string> Live2DModelManager::get_registered_models() const {
    std::vector<std::string> models;
    models.reserve(m_registered_models.size());

    for (const auto& pair : m_registered_models) {
        models.push_back(pair.first);
    }

    return models;
}

std::vector<std::string> Live2DModelManager::get_loaded_models() const {
    std::vector<std::string> models;
    models.reserve(m_loaded_models.size());

    for (const auto& pair : m_loaded_models) {
        models.push_back(pair.first);
    }

    return models;
}

const ModelRegistration* Live2DModelManager::get_model_info(const std::string& model_id) const {
    auto it = m_registered_models.find(model_id);
    if (it == m_registered_models.end()) {
        return nullptr;
    }
    return &it->second;
}

void Live2DModelManager::clear_all_registrations() {
    LOG_INFO("Live2DModelManager: Clearing all registrations");

    // 先卸载所有模型
    unload_all_models();

    // 清除注册信息
    m_registered_models.clear();

    LOG_INFO("Live2DModelManager: All registrations cleared");
}

// ============================================================================
// 辅助方法
// ============================================================================

std::vector<std::string> Live2DModelManager::find_model_json_files(
    const std::string& directory,
    bool recursive
) const {
    std::vector<std::string> model_files;

    try {
        fs::path dir_path(directory);

        if (recursive) {
            // 递归扫描
            for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                if (entry.is_regular_file() &&
                    entry.path().extension() == ".json" &&
                    entry.path().string().ends_with(".model3.json")) {
                    model_files.push_back(entry.path().string());
                }
            }
        } else {
            // 非递归扫描
            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (entry.is_regular_file() &&
                    entry.path().extension() == ".json" &&
                    entry.path().string().ends_with(".model3.json")) {
                    model_files.push_back(entry.path().string());
                }
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Live2DModelManager: Error scanning directory '{}': {}",
                  directory, e.what());
    }

    return model_files;
}

std::string Live2DModelManager::extract_model_id(const std::string& json_path) const {
    // 从文件路径提取模型 ID
    // 例如: "models/live2d/Haru/Haru.model3.json" -> "Haru"

    fs::path path(json_path);
    std::string filename = path.stem().string(); // 去掉 .model3.json 后缀

    // 如果有 .model3 后缀，去掉它
    if (filename.ends_with(".model3")) {
        filename = filename.substr(0, filename.size() - 7);
    }

    return filename;
}

} // namespace DearTs::Plugins::Live2D
