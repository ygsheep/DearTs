#pragma once

#include "layout_base.h"
#include <string>
#include <imgui.h>
#include <chrono>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 番茄时钟布局类
 * 实现番茄工作法的计时器功能
 */
class PomodoroLayout : public LayoutBase {
public:
    /**
     * @brief 构造函数
     */
    explicit PomodoroLayout();
    
    /**
     * @brief 渲染番茄时钟布局
     */
    void render() override;
    
    /**
     * @brief 更新番茄时钟布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    void updateLayout(float width, float height) override;
    
    /**
     * @brief 处理番茄时钟事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;

    /**
     * @brief 在固定区域内渲染内容
     * @param contentX 内容区域X坐标
     * @param contentY 内容区域Y坐标
     * @param contentWidth 内容区域宽度
     * @param contentHeight 内容区域高度
     */
    void renderInFixedArea(float contentX, float contentY, float contentWidth, float contentHeight) override;

    /**
     * @brief 设置是否显示布局
     */
    void setVisible(bool visible);
    
    /**
     * @brief 检查是否可见
     */
    bool isVisible() const { return isVisible_; }
    
    /**
     * @brief 测试方法：开始计时器
     */
    void testStartTimer() { startTimer(); }
    
    /**
     * @brief 测试方法：检查是否正在运行
     */
    bool isRunning() const { return isRunning_; }
    
    /**
     * @brief 测试方法：获取剩余时间
     */
    int getRemainingTime() const { return remainingTime_; }

private:
    bool isVisible_;                ///< 是否可见
    int workDuration_;              ///< 工作时长（秒）
    int breakDuration_;             ///< 休息时长（秒）
    int remainingTime_;             ///< 剩余时间（秒）
    bool isRunning_;                ///< 是否正在运行
    bool isWorkMode_;               ///< 是否为工作模式
    std::string currentModeText_;   ///< 当前模式文本

    // 计时器相关成员变量
    std::chrono::high_resolution_clock::time_point lastUpdateTime_;  ///< 上次更新时间
    double accumulatedTime_;         ///< 累积时间
    
    /**
     * @brief 显示Windows通知
     */
    void showNotification(const std::string& title, const std::string& message);
    
    /**
     * @brief 开始计时器
     */
    void startTimer();
    
    /**
     * @brief 暂停计时器
     */
    void pauseTimer();
    
    /**
     * @brief 重置计时器
     */
    void resetTimer();
    
    /**
     * @brief 切换模式（工作/休息）
     */
    void switchMode();
    
    /**
     * @brief 更新计时器状态
     */
    void updateTimer();
    
    /**
     * @brief 格式化时间显示
     */
    std::string formatTime(int seconds) const;
};

} // namespace Window
} // namespace Core
} // namespace DearTs