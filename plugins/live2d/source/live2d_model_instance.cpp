/**
 * @file live2d_model_instance.cpp
 * @brief Live2D 模型实例实现
 */

#include "live2d_model_instance.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <CubismModelSettingJson.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Motion/CubismMotion.hpp>
#include <Physics/CubismPhysics.hpp>
#include <Effect/CubismBreath.hpp>
#include <Effect/CubismEyeBlink.hpp>
#include <Id/CubismIdManager.hpp>
#include <Utils/CubismString.hpp>

// 取消 Windows 宏定义，避免与 logger 冲突
#ifdef ERROR
#undef ERROR
#endif

#include "liblogger/logger.h"
#include <fstream>
#include <vector>
#include <filesystem>

namespace DearTs::Plugins::Live2D {

namespace fs = std::filesystem;

// ============================================================================
// 构造函数和析构函数
// ============================================================================

Live2DModelInstance::Live2DModelInstance()
    : L2D::CubismUserModel()
{
}

Live2DModelInstance::~Live2DModelInstance() {
    unload();

    // 删除模型设置
    if (_modelSetting) {
        delete _modelSetting;
        _modelSetting = nullptr;
    }
}

// ============================================================================
// CubismUserModel 虚方法实现
// ============================================================================

L2D::csmByte* Live2DModelInstance::CreateBuffer(const L2D::csmChar* path, L2D::csmSizeInt* size) {
    if (!path || !size) {
        return nullptr;
    }

    // 打开文件
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Live2DModelInstance: Failed to open file: {}", path);
        return nullptr;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    *size = static_cast<L2D::csmSizeInt>(fileSize);

    // 读取文件
    L2D::csmByte* buffer = new Csm::csmByte[*size];
    file.read(reinterpret_cast<char*>(buffer), *size);
    file.close();

    LOG_DEBUG("Live2DModelInstance: Loaded file: {} ({} bytes)", path, *size);
    return buffer;
}

void Live2DModelInstance::DeleteBuffer(L2D::csmByte* buffer, const L2D::csmChar* path) {
    if (buffer) {
        delete[] buffer;
    }
    if (path) {
        LOG_DEBUG("Live2DModelInstance: Released buffer for: {}", path);
    }
}

// ============================================================================
// 模型加载
// ============================================================================

Result<void, std::string> Live2DModelInstance::load_from_directory(
    const fs::path& model_dir
) {
    LOG_INFO("Live2DModelInstance: Scanning directory for .model3.json: {}",
             model_dir.string());

    // 查找 .model3.json 文件
    for (const auto& entry : fs::directory_iterator(model_dir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.ends_with(".model3.json")) {
                LOG_INFO("Live2DModelInstance: Found .model3.json: {}", filename);
                return load_from_model3_json(entry.path());
            }
        }
    }

    std::string error_msg = "No .model3.json file found in directory: ";
    error_msg += model_dir.string();
    return Result<void, std::string>::err(error_msg);
}

Result<void, std::string> Live2DModelInstance::load_from_model3_json(
    const fs::path& model3_json_path
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
             model3_json_path.string());

    try {
        // 设置模型目录
        m_model_directory = model3_json_path.parent_path();

        // 读取 .model3.json 文件
        L2D::csmSizeInt size;
        const L2D::csmString path = model3_json_path.string().c_str();
        L2D::csmByte* buffer = CreateBuffer(path.GetRawString(), &size);

        if (!buffer) {
            std::string error_msg = "Failed to read .model3.json file: ";
            error_msg += model3_json_path.string();
            m_load_state = ModelLoadState::Error;
            m_last_error = error_msg;
            return Result<void, std::string>::err(error_msg);
        }

        // 创建 CubismModelSettingJson
        _modelSetting = new L2D::CubismModelSettingJson(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());

        // 设置模型
        auto result = setup_model(_modelSetting);
        if (result.isErr()) {
            m_load_state = ModelLoadState::Error;
            m_last_error = result.error();
            return result;
        }

        // 创建渲染器
        if (_model) {
            CreateRenderer();
        } else {
            std::string error_msg = "Failed to create model object";
            m_load_state = ModelLoadState::Error;
            m_last_error = error_msg;
            return Result<void, std::string>::err(error_msg);
        }

        // 加载纹理
        auto tex_result = setup_textures();
        if (tex_result.isErr()) {
            m_load_state = ModelLoadState::Error;
            m_last_error = tex_result.error();
            return tex_result;
        }

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

Result<void, std::string> Live2DModelInstance::setup_model(L2D::ICubismModelSetting* setting) {
    _updating = true;
    _initialized = false;

    L2D::csmByte* buffer;
    L2D::csmSizeInt size;

    // 加载 MOC3 文件
    if (strcmp(setting->GetModelFileName(), "") != 0) {
        L2D::csmString path = setting->GetModelFileName();
        // 添加路径分隔符
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

        LOG_INFO("Live2DModelInstance: Loading MOC3: {}", fullPath.GetRawString());

        buffer = CreateBuffer(fullPath.GetRawString(), &size);
        LoadModel(buffer, size, false);  // 不检查 MOC 一致性以提高性能
        DeleteBuffer(buffer, fullPath.GetRawString());
    }

    // 加载表情文件
    if (setting->GetExpressionCount() > 0) {
        const L2D::csmInt32 count = setting->GetExpressionCount();
        for (L2D::csmInt32 i = 0; i < count; i++) {
            L2D::csmString name = setting->GetExpressionName(i);
            L2D::csmString path = setting->GetExpressionFileName(i);
            L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

            LOG_DEBUG("Live2DModelInstance: Loading expression: {}", fullPath.GetRawString());

            buffer = CreateBuffer(fullPath.GetRawString(), &size);
            L2D::ACubismMotion* motion = LoadExpression(buffer, size, name.GetRawString());
            DeleteBuffer(buffer, fullPath.GetRawString());
        }
    }

    // 加载物理文件
    if (strcmp(setting->GetPhysicsFileName(), "") != 0) {
        L2D::csmString path = setting->GetPhysicsFileName();
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

        LOG_INFO("Live2DModelInstance: Loading physics: {}", fullPath.GetRawString());

        buffer = CreateBuffer(fullPath.GetRawString(), &size);
        LoadPhysics(buffer, size);
        DeleteBuffer(buffer, fullPath.GetRawString());
    }

    // 加载姿势文件
    if (strcmp(setting->GetPoseFileName(), "") != 0) {
        L2D::csmString path = setting->GetPoseFileName();
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

        LOG_INFO("Live2DModelInstance: Loading pose: {}", fullPath.GetRawString());

        buffer = CreateBuffer(fullPath.GetRawString(), &size);
        LoadPose(buffer, size);
        DeleteBuffer(buffer, fullPath.GetRawString());
    }

    // 眨眼
    if (setting->GetEyeBlinkParameterCount() > 0) {
        _eyeBlink = Csm::CubismEyeBlink::Create(setting);
        m_info.has_eye_blink = true;
    }

    // 呼吸
    {
        _breath = Csm::CubismBreath::Create();
        m_info.has_breath = true;

        L2D::csmVector<L2D::CubismBreath::BreathParameterData> breathParameters;

        // 使用默认呼吸参数
        breathParameters.PushBack(L2D::CubismBreath::BreathParameterData(
            L2D::CubismFramework::GetIdManager()->GetId("ParamAngleX"), 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(L2D::CubismBreath::BreathParameterData(
            L2D::CubismFramework::GetIdManager()->GetId("ParamAngleY"), 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(L2D::CubismBreath::BreathParameterData(
            L2D::CubismFramework::GetIdManager()->GetId("ParamAngleZ"), 0.0f, 10.0f, 5.5345f, 0.5f));

        _breath->SetParameters(breathParameters);
    }

    // 用户数据
    if (strcmp(setting->GetUserDataFile(), "") != 0) {
        L2D::csmString path = setting->GetUserDataFile();
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

        LOG_DEBUG("Live2DModelInstance: Loading user data: {}", fullPath.GetRawString());

        buffer = CreateBuffer(fullPath.GetRawString(), &size);
        LoadUserData(buffer, size);
        DeleteBuffer(buffer, fullPath.GetRawString());
    }

    if (!_model || !_modelMatrix) {
        std::string error_msg = "Failed to initialize model";
        LOG_ERROR("Live2DModelInstance: {}", error_msg);
        _updating = false;
        return Result<void, std::string>::err(error_msg);
    }

    // 布局
    L2D::csmMap<L2D::csmString, L2D::csmFloat32> layout;
    setting->GetLayoutMap(layout);

    if (layout.GetSize() > 0) {
        // 使用模型文件中的布局信息
        _modelMatrix->SetupFromLayout(layout);
        LOG_INFO("Live2DModelInstance: Applied model layout ({} items)", layout.GetSize());
    } else {
        // 没有布局信息，设置默认值以适应视口
        LOG_WARN("Live2DModelInstance: No layout info in model JSON, using default layout");

        // 获取模型Canvas尺寸
        const L2D::CubismModel* model = _model;
        float canvasWidth = model->GetCanvasWidth();
        float canvasHeight = model->GetCanvasHeight();

        LOG_INFO("Live2DModelInstance: Model canvas size: {}x{}", canvasWidth, canvasHeight);

        // 设置模型居中并适当缩放
        _modelMatrix->SetupFromLayout(layout);  // 使用空布局
        _modelMatrix->SetWidth(2.0f);  // 设置宽度比例
        _modelMatrix->SetHeight(2.0f);  // 设置高度比例
    }

    _model->SaveParameters();

    // 预加载动作组
    for (L2D::csmInt32 i = 0; i < setting->GetMotionGroupCount(); i++) {
        const L2D::csmChar* group = setting->GetMotionGroupName(i);
        preload_motion_group(group);
    }

    _motionManager->StopAllMotions();

    _updating = false;
    _initialized = true;

    return Result<void, std::string>::ok();
}

Result<void, std::string> Live2DModelInstance::setup_textures() {
    LOG_INFO("Live2DModelInstance: Loading textures");

    auto* renderer = GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>();
    if (!renderer) {
        return Result<void, std::string>::err("Failed to get renderer");
    }

    for (L2D::csmInt32 modelTextureNumber = 0;
         modelTextureNumber < _modelSetting->GetTextureCount();
         modelTextureNumber++)
    {
        // 纹理名为空则跳过
        if (strcmp(_modelSetting->GetTextureFileName(modelTextureNumber), "") == 0) {
            continue;
        }

        // 获取纹理路径
        L2D::csmString texturePath = _modelSetting->GetTextureFileName(modelTextureNumber);
        // 添加路径分隔符
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + texturePath;

        LOG_INFO("Live2DModelInstance: Loading texture {}: {}",
                 modelTextureNumber, fullPath.GetRawString());

        // 使用 SDL_image 加载纹理
        SDL_Surface* surface = IMG_Load(fullPath.GetRawString());
        if (!surface) {
            LOG_ERROR("Live2DModelInstance: Failed to load texture: {}", fullPath.GetRawString());
            continue;
        }

        // 转换为 RGBA 格式 (SDL3 API)
        SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);

        if (!rgbaSurface) {
            LOG_ERROR("Live2DModelInstance: Failed to convert surface: {}", fullPath.GetRawString());
            continue;
        }

        // 创建 OpenGL 纹理
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // 设置纹理参数
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     rgbaSurface->w, rgbaSurface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgbaSurface->pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        SDL_DestroySurface(rgbaSurface);

        // 绑定纹理到渲染器
        renderer->BindTexture(modelTextureNumber, textureId);

        LOG_INFO("Live2DModelInstance: Texture loaded: {} (OpenGL ID: {})",
                 fullPath.GetRawString(), textureId);
    }

    // 设置预乘 Alpha
    renderer->IsPremultipliedAlpha(false);

    return Result<void, std::string>::ok();
}

void Live2DModelInstance::preload_motion_group(const L2D::csmChar* group) {
    const L2D::csmInt32 count = _modelSetting->GetMotionCount(group);

    for (L2D::csmInt32 i = 0; i < count; i++) {
        L2D::csmString name = L2D::Utils::CubismString::GetFormatedString("%s_%d", group, i);
        L2D::csmString path = _modelSetting->GetMotionFileName(group, i);
        // 添加路径分隔符
        L2D::csmString fullPath = L2D::csmString(m_model_directory.string().c_str()) + "/" + path;

        LOG_DEBUG("Live2DModelInstance: Loading motion: {}", fullPath.GetRawString());

        L2D::csmByte* buffer;
        L2D::csmSizeInt size;
        buffer = CreateBuffer(fullPath.GetRawString(), &size);

        L2D::CubismMotion* motion = static_cast<L2D::CubismMotion*>(
            LoadMotion(buffer, size, name.GetRawString(), NULL, NULL, _modelSetting, group, i, false));

        DeleteBuffer(buffer, fullPath.GetRawString());
    }

    m_info.has_motion = true;
}

void Live2DModelInstance::unload() {
    if (m_load_state == ModelLoadState::Unloaded) {
        return;
    }

    LOG_INFO("Live2DModelInstance: Unloading model: {}", m_info.model_name);

    // CubismUserModel 会自动清理资源
    DeleteRenderer();

    // 删除模型设置
    if (_modelSetting) {
        delete _modelSetting;
        _modelSetting = nullptr;
    }

    m_info = ModelInfo{};
    m_load_state = ModelLoadState::Unloaded;

    LOG_INFO("Live2DModelInstance: Model unloaded");
}

// ============================================================================
// 更新和渲染
// ============================================================================

void Live2DModelInstance::update(float delta_time) {
    if (!is_loaded() || !_model) {
        return;
    }

    // 保存参数
    _model->LoadParameters();

    // 更新动作
    if (_motionManager) {
        _motionManager->UpdateMotion(_model, delta_time);
    }

    // 保存参数
    _model->SaveParameters();

    // 更新不透明度
    _opacity = _model->GetModelOpacity();

    // 眨眼
    if (_eyeBlink) {
        _eyeBlink->UpdateParameters(_model, delta_time);
    }

    // 表情
    if (_expressionManager) {
        _expressionManager->UpdateMotion(_model, delta_time);
    }

    // 呼吸
    if (_breath) {
        _breath->UpdateParameters(_model, delta_time);
    }

    // 物理
    if (_physics) {
        _physics->Evaluate(_model, delta_time);
    }

    // 姿势
    if (_pose) {
        _pose->UpdateParameters(_model, delta_time);
    }

    // 更新模型
    _model->Update();
}

void Live2DModelInstance::draw() {
    LOG_INFO("Live2DModelInstance: draw() called for model '{}'", m_info.model_name);

    if (!is_loaded() || !_model) {
        LOG_DEBUG("Live2DModelInstance: Skipping draw (model not loaded)");
        return;
    }
    LOG_DEBUG("Live2DModelInstance: Model is loaded and _model is not null");

    // 确保 OpenGL 上下文存在
    if (!glGetString(GL_VERSION)) {
        LOG_ERROR("Live2DModelInstance: No OpenGL context, skipping draw");
        return;
    }
    LOG_DEBUG("Live2DModelInstance: OpenGL context exists");

    auto* renderer = GetRenderer<Csm::Rendering::CubismRenderer_OpenGLES2>();
    if (!renderer) {
        LOG_ERROR("Live2DModelInstance: Failed to get renderer for model '{}'",
                  m_info.model_name);
        return;
    }
    LOG_DEBUG("Live2DModelInstance: Got renderer pointer");

    // 确保 renderer 已初始化
    if (!renderer->GetModel()) {
        LOG_ERROR("Live2DModelInstance: Renderer not initialized for model '{}'",
                  m_info.model_name);
        return;
    }
    LOG_DEBUG("Live2DModelInstance: Renderer is initialized");

    LOG_DEBUG("Live2DModelInstance: Drawing model '{}', size: {}x{}, canvas: {}x{}",
              m_info.model_name, m_info.width, m_info.height,
              _model->GetCanvasWidth(), _model->GetCanvasHeight());

    // 更新模型矩阵
    if (_modelMatrix) {
        LOG_DEBUG("Live2DModelInstance: Setting MVP matrix");
        renderer->SetMvpMatrix(_modelMatrix);
    } else {
        LOG_WARN("Live2DModelInstance: Model matrix is null");
    }

    LOG_DEBUG("Live2DModelInstance: Calling DrawModel()");
    renderer->DrawModel();
    LOG_DEBUG("Live2DModelInstance: DrawModel() completed");
}

bool Live2DModelInstance::is_valid() const {
    return is_loaded() && _model != nullptr;
}

// ============================================================================
// 参数控制
// ============================================================================

void Live2DModelInstance::set_parameter(const std::string& parameter_name, float value) {
    if (!_model) {
        return;
    }

    const Csm::CubismId* id = L2D::CubismFramework::GetIdManager()->GetId(parameter_name.c_str());
    _model->SetParameterValue(id, value);
}

float Live2DModelInstance::get_parameter(const std::string& parameter_name) const {
    if (!_model) {
        return 0.0f;
    }

    const Csm::CubismId* id = L2D::CubismFramework::GetIdManager()->GetId(parameter_name.c_str());
    return _model->GetParameterValue(id);
}

void Live2DModelInstance::add_parameter(const std::string& parameter_name, float value) {
    if (!_model) {
        return;
    }

    const Csm::CubismId* id = L2D::CubismFramework::GetIdManager()->GetId(parameter_name.c_str());
    _model->AddParameterValue(id, value);
}

// ============================================================================
// 部位透明度
// ============================================================================

void Live2DModelInstance::set_part_opacity(int part_index, float opacity) {
    if (!_model) {
        return;
    }

    _model->SetPartOpacity(part_index, opacity);
}

float Live2DModelInstance::get_part_opacity(int part_index) const {
    if (!_model) {
        return 0.0f;
    }

    return _model->GetPartOpacity(part_index);
}

// ============================================================================
// 私有方法
// ============================================================================

void Live2DModelInstance::setup_model_info() {
    if (!_model || !_modelSetting) {
        return;
    }

    // 设置模型名称
    m_info.model_name = m_model_directory.filename().string();
    m_info.model_path = m_model_directory.string();

    // 设置模型尺寸
    m_info.width = static_cast<int>(_model->GetCanvasWidthPixel());
    m_info.height = static_cast<int>(_model->GetCanvasHeightPixel());

    // 默认值（如果获取失败）
    if (m_info.width == 0) m_info.width = 512;
    if (m_info.height == 0) m_info.height = 512;

    // 检查是否有物理
    m_info.has_physics = (_physics != nullptr);

    LOG_INFO("Live2DModelInstance: Model info - Name: {}, Size: {}x{}, Physics: {}, Motion: {}, EyeBlink: {}, Breath: {}",
             m_info.model_name, m_info.width, m_info.height,
             m_info.has_physics, m_info.has_motion,
             m_info.has_eye_blink, m_info.has_breath);
}

} // namespace DearTs::Plugins::Live2D
