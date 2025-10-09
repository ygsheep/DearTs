#include "pomodoro_layout.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include "../utils/logger.h"
#include "../window_base.h"
#include "../resource/font_resource.h"

// WinToast库用于Windows通知
#ifdef _WIN32
#include <windows.h>
#include "wintoastlib.h"
#endif

namespace DearTs {
  namespace Core {
    namespace Window {

      // Toast处理器类 - 提前定义以便静态使用
      class PomodoroToastHandler : public WinToastLib::IWinToastHandler {
      public:
          void toastActivated() const override {
              DEARTS_LOG_INFO("Toast notification activated");
          }
          void toastActivated(int actionIndex) const override {
              DEARTS_LOG_INFO("Toast notification action activated: " + std::to_string(actionIndex));
          }
          void toastActivated(std::wstring response) const override {
              DEARTS_LOG_INFO("Toast notification response: " + std::string(response.begin(), response.end()));
          }
          void toastDismissed(WinToastDismissalReason state) const override {
              DEARTS_LOG_INFO("Toast notification dismissed");
          }
          void toastFailed() const override {
              DEARTS_LOG_ERROR("Toast notification failed");
          }
      };

      /**
       * PomodoroLayout构造函数
       */
      PomodoroLayout::PomodoroLayout() :
          LayoutBase("Pomodoro"), isVisible_(false), workDuration_(25 * 60), breakDuration_(5 * 60), remainingTime_(0),
          isRunning_(false), isWorkMode_(true), currentModeText_("工作模式"), accumulatedTime_(0.0) {
        remainingTime_ = workDuration_;
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();
      }

      /**
       * 渲染番茄时钟布局
       */
      void PomodoroLayout::render() {
        if (!isVisible_) {
          return;
        }

        // 显示当前模式
        ImGui::Text("当前模式: %s", currentModeText_.c_str());
        ImGui::Separator();

        // 显示倒计时（使用大字体）
        std::string timeText = formatTime(remainingTime_);
        ImVec2 availableSize = ImGui::GetContentRegionAvail();

        // 使用FontManager获取大字体显示时间
        auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
        auto largeFont = fontManager ? fontManager->loadLargeFont(32.0f) : nullptr;

        std::shared_ptr<DearTs::Core::Resource::FontResource> usedFont = nullptr;
        if (largeFont) {
            largeFont->pushFont();
            usedFont = largeFont;
        }

        // 计算文本位置并居中显示
        ImVec2 textSize = ImGui::CalcTextSize(timeText.c_str());
        ImGui::SetCursorPosX((availableSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY(50);
        ImGui::Text("%s", timeText.c_str());

        // 恢复字体
        if (usedFont) {
            usedFont->popFont();
        }

        ImGui::Dummy(ImVec2(0.0f, 30.0f));

        // 控制按钮
        ImVec2 buttonSize(80, 30);
        float buttonsWidth = buttonSize.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        ImGui::SetCursorPosX((availableSize.x - buttonsWidth) * 0.5f);

        if (ImGui::Button(isRunning_ ? "暂停" : "开始", buttonSize)) {
          if (isRunning_) {
            pauseTimer();
          } else {
            startTimer();
          }
        }

        ImGui::SameLine();
        if (ImGui::Button("重置", buttonSize)) {
          resetTimer();
        }

        ImGui::SameLine();
        if (ImGui::Button("切换", buttonSize)) {
          switchMode();
        }

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        // 设置工作和休息时间
        ImGui::Separator();
        ImGui::Text("设置时间:");
        ImGui::PushItemWidth(100);

        static int workMinutes = 25;
        static int breakMinutes = 5;

        if (ImGui::InputInt("工作时间(分钟)", &workMinutes, 1, 5)) {
          workMinutes = std::max(1, std::min(60, workMinutes));
          workDuration_ = workMinutes * 60;
          if (!isRunning_ && isWorkMode_) {
            remainingTime_ = workDuration_;
          }
        }

        if (ImGui::InputInt("休息时间(分钟)", &breakMinutes, 1, 5)) {
          breakMinutes = std::max(1, std::min(60, breakMinutes));
          breakDuration_ = breakMinutes * 60;
          if (!isRunning_ && !isWorkMode_) {
            remainingTime_ = breakDuration_;
          }
        }

        ImGui::PopItemWidth();

        // 显示进度条
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        float progress = 0.0f;
        int totalTime = isWorkMode_ ? workDuration_ : breakDuration_;
        if (totalTime > 0) {
          progress = 1.0f - (float) remainingTime_ / totalTime;
        }
        ImVec2 progressBarSize(availableSize.x - 20, 10);
        ImGui::SetCursorPosX(10);
        ImGui::ProgressBar(progress, progressBarSize);
      }

      /**
       * 更新番茄时钟布局
       */
      void PomodoroLayout::updateLayout(float width, float height) {
        // updateLayout方法被频繁调用，移除冗余日志输出

        // 更新计时器
        updateTimer();

        // 更新位置和大小
        setPosition(300, 100);
        setSize(400, 300);
      }

      /**
       * 处理番茄时钟事件
       */
      void PomodoroLayout::handleEvent(const SDL_Event &event) {
        // 在这里可以添加自定义事件处理逻辑
        // 注意：ImGui事件处理通常在主窗口中统一处理
      }

      /**
       * 开始计时器
       */
      void PomodoroLayout::startTimer() {
        isRunning_ = true;
        accumulatedTime_ = 0.0;  // 重置累积时间
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();  // 重置时间基准
        DEARTS_LOG_INFO("番茄时钟开始计时");

        // 显示开始通知
        if (isWorkMode_) {
          showNotification("番茄时钟", "开始工作时间！");
        } else {
          showNotification("番茄时钟", "开始休息时间！");
        }
      }

      void PomodoroLayout::setVisible(bool visible) {
        isVisible_ = visible;
        DEARTS_LOG_INFO("PomodoroLayout::setVisible() 设置为: " + std::string(visible ? "true" : "false"));
      }

      /**
       * 暂停计时器
       */
      void PomodoroLayout::pauseTimer() {
        isRunning_ = false;
        DEARTS_LOG_INFO("番茄时钟暂停计时");
      }

      /**
       * 重置计时器
       */
      void PomodoroLayout::resetTimer() {
        isRunning_ = false;
        remainingTime_ = isWorkMode_ ? workDuration_ : breakDuration_;
        accumulatedTime_ = 0.0;  // 重置累积时间
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();  // 重置时间基准
        DEARTS_LOG_INFO("番茄时钟重置计时器");
      }

      /**
       * 切换模式（工作/休息）
       */
      void PomodoroLayout::switchMode() {
        isWorkMode_ = !isWorkMode_;
        currentModeText_ = isWorkMode_ ? "工作模式" : "休息模式";
        remainingTime_ = isWorkMode_ ? workDuration_ : breakDuration_;
        isRunning_ = false;
        accumulatedTime_ = 0.0;  // 重置累积时间
        lastUpdateTime_ = std::chrono::high_resolution_clock::now();  // 重置时间基准
        DEARTS_LOG_INFO("番茄时钟切换模式: " + currentModeText_);
      }

      /**
       * 显示Windows通知
       */
      void PomodoroLayout::showNotification(const std::string& title, const std::string& message) {
#ifdef _WIN32
        try {
            // 静态初始化标志，避免重复初始化
            static bool winToastInitialized = false;
            static bool initializationAttempted = false;
            static std::wstring lastError;

            // 如果初始化失败过，不再尝试
            if (initializationAttempted && !winToastInitialized) {
                return;
            }

            // 获取WinToast实例
            auto winToast = WinToastLib::WinToast::instance();

            // 首次尝试初始化
            if (!winToastInitialized) {
                initializationAttempted = true;

                // 检查是否支持Toast通知
                if (!WinToastLib::WinToast::isCompatible()) {
                    DEARTS_LOG_WARN("Windows Toast notifications not supported on this system");
                    lastError = L"System not compatible";
                    return;
                }

                // 设置应用名称和AUMI
                winToast->setAppName(L"DearTs Pomodoro Timer");
                std::wstring aumi = WinToastLib::WinToast::configureAUMI(
                    L"DearTs",
                    L"PomodoroTimer",
                    L"1.0.0"
                );
                winToast->setAppUserModelId(aumi);

                // 初始化WinToast
                WinToastLib::WinToast::WinToastError error;
                if (!winToast->initialize(&error)) {
                    DEARTS_LOG_ERROR("Failed to initialize WinToast: " + std::to_string(static_cast<int>(error)));
                    lastError = L"Initialization failed";
                    return;
                }

                winToastInitialized = true;
                DEARTS_LOG_INFO("WinToast initialized successfully");
            }

            // 转换为宽字符 - 使用正确的UTF-8到宽字符转换
            std::wstring wtitle;
            std::wstring wmessage;

            // 将UTF-8字符串转换为宽字符
            int titleLength = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            if (titleLength > 0) {
                wtitle.resize(titleLength - 1); // 减去null终止符
                MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], titleLength);
            }

            int messageLength = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
            if (messageLength > 0) {
                wmessage.resize(messageLength - 1); // 减去null终止符
                MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wmessage[0], messageLength);
            }

            // 静态处理器实例，避免重复创建
            static PomodoroToastHandler* handler = nullptr;
            if (handler == nullptr) {
                handler = new PomodoroToastHandler();
            }

            // 创建Toast模板
            WinToastLib::WinToastTemplate templ(WinToastLib::WinToastTemplate::WinToastTemplateType::Text02);
            templ.setTextField(wtitle, WinToastLib::WinToastTemplate::FirstLine);    // 标题
            templ.setTextField(wmessage, WinToastLib::WinToastTemplate::SecondLine);  // 消息内容

            // 设置通知持续时间（5秒）
            templ.setExpiration(5000);

            // 显示通知
            WinToastLib::WinToast::WinToastError error;
            INT64 notificationId = winToast->showToast(templ, handler, &error);

            if (notificationId < 0) {
                DEARTS_LOG_ERROR("Failed to show notification");
            } else {
                DEARTS_LOG_INFO("Toast notification sent successfully: " + title + " - " + message);
            }

        } catch (const std::exception& e) {
            DEARTS_LOG_ERROR("Exception in showNotification: " + std::string(e.what()));
        } catch (...) {
            DEARTS_LOG_ERROR("Unknown exception in showNotification");
        }
#else
        // 非Windows平台，输出到日志
        DEARTS_LOG_INFO("Notification: [" + title + "] " + message);
#endif
      }

      /**
       * 更新计时器状态
       */
      void PomodoroLayout::updateTimer() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastUpdateTime_).count();
        lastUpdateTime_ = currentTime;

        // 只有在可见且运行时才更新计时器
        if (isVisible_ && isRunning_) {
          accumulatedTime_ += deltaTime;

          // 每秒更新一次
          if (accumulatedTime_ >= 1.0) {
            int secondsToSubtract = (int)accumulatedTime_;
            remainingTime_ -= secondsToSubtract;
            accumulatedTime_ -= secondsToSubtract;

            // 只在倒计时有显著变化时记录日志
            DEARTS_LOG_INFO("番茄时钟倒计时更新 - 剩余时间: " + std::to_string(remainingTime_) + "秒");

            // 检查是否到达0
            if (remainingTime_ <= 0) {
              remainingTime_ = 0;
              isRunning_ = false;

              // 显示通知
              if (isWorkMode_) {
                showNotification("番茄时钟", "工作时间结束，开始休息吧！");
              } else {
                showNotification("番茄时钟", "休息时间结束，开始工作吧！");
              }

              // 计时结束，切换模式
              switchMode();
            }
          }
        }
      }

      /**
       * 格式化时间显示
       */
      std::string PomodoroLayout::formatTime(int seconds) const {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, secs);
        return std::string(buffer);
      }

    } // namespace Window
  } // namespace Core
} // namespace DearTs