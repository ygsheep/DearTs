/**
 * @file ilive2d_renderer.hpp
 * @brief Live2D 渲染器抽象接口
 * @details 定义 Live2D 渲染器的通用接口，支持不同的图形 API
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include <cstdint>
#include <memory>
#include <string>

namespace DearTs::Plugins::Live2D {

// 导入 Result 类型到当前命名空间
using DearTs::Core::Result;

/**
 * @brief Live2D 渲染器状态
 */
enum class RendererState {
    Uninitialized,   ///< 未初始化
    Initialized,     ///< 已初始化
    Rendering,       ///< 正在渲染
    Error            ///< 错误状态
};

/**
 * @brief 渲染器配置
 */
struct RendererConfig {
    int viewport_width = 800;      ///< 视口宽度
    int viewport_height = 600;     ///< 视口高度
    bool use_fbo = false;          ///< 是否使用帧缓冲对象
    int fbo_downsample = 1;        ///< FBO 降采样比例 (1-4)
    bool enable_profiling = false; ///< 是否启用性能分析
};

/**
 * @brief Live2D 渲染器抽象接口
 *
 * @details
 * 定义了 Live2D 渲染器的核心功能：
 * - 初始化和清理
 * - 模型加载和卸载
 * - 渲染控制
 * - 性能监控
 *
 * @note
 * 此接口与具体图形 API 解耦，支持 OpenGL、DirectX 等不同实现
 */
class ILive2DRenderer {
public:
    virtual ~ILive2DRenderer() = default;

    /**
     * @brief 初始化渲染器
     * @param config 渲染器配置
     * @return 成功返回 void，失败返回错误信息
     */
    virtual Result<void, std::string> initialize(const RendererConfig& config) = 0;

    /**
     * @brief 关闭渲染器并释放资源
     */
    virtual void shutdown() = 0;

    /**
     * @brief 获取渲染器状态
     */
    virtual RendererState get_state() const = 0;

    /**
     * @brief 开始渲染帧
     * @details 准备渲染环境，清除缓冲区等
     */
    virtual void begin_frame() = 0;

    /**
     * @brief 结束渲染帧
     * @details 完成渲染，交换缓冲区等
     */
    virtual void end_frame() = 0;

    /**
     * @brief 设置视口大小
     * @param width 视口宽度
     * @param height 视口高度
     */
    virtual void set_viewport(int width, int height) = 0;

    /**
     * @brief 获取视口宽度
     */
    virtual int get_viewport_width() const = 0;

    /**
     * @brief 获取视口高度
     */
    virtual int get_viewport_height() const = 0;

    /**
     * @brief 设置清除颜色
     * @param r 红色分量 (0.0-1.0)
     * @param g 绿色分量 (0.0-1.0)
     * @param b 蓝色分量 (0.0-1.0)
     * @param a 透明度分量 (0.0-1.0)
     */
    virtual void set_clear_color(float r, float g, float b, float a = 1.0f) = 0;

    /**
     * @brief 获取性能统计信息
     * @return JSON 格式的性能数据
     */
    virtual std::string get_profiling_data() const = 0;

    /**
     * @brief 检查渲染器是否可用
     */
    virtual bool is_valid() const = 0;
};

} // namespace DearTs::Plugins::Live2D
