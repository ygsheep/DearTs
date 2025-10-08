#include "pomodoro_layout.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include "../utils/logger.h"
#include "../window_base.h"
#include "../resource/font_resource.h"

// Windows头文件用于通知
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#endif

namespace DearTs {
  namespace Core {
    namespace Window {

      /**
       * PomodoroLayout构造函数
       */
      PomodoroLayout::PomodoroLayout() :
          LayoutBase("Pomodoro"), isVisible_(false), workDuration_(25 * 60), breakDuration_(5 * 60), remainingTime_(0),
          isRunning_(false), isWorkMode_(true), currentModeText_("工作模式") {
        remainingTime_ = workDuration_;
      }

      /**
       * 渲染番茄时钟布局
       */
      void PomodoroLayout::render() {
        if (!isVisible_) {
          return;
        }

        // 显示当前模式
        ImGui::SetWindowFontScale(1.2f); // 适度放大模式文本
        ImGui::Text("当前模式: %s", currentModeText_.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();

        // 显示倒计时（使用大字体）
        std::string timeText = formatTime(remainingTime_);
        ImVec2 textSize = ImGui::CalcTextSize(timeText.c_str());
        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        
        // 使用更大的字体显示时间
        // 为了避免字体模糊，我们不使用SetWindowFontScale，而是直接使用更大的字体
        auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
        std::shared_ptr<DearTs::Core::Resource::FontResource> usedFont = nullptr;
        
        if (fontManager) {
            // 尝试获取一个更大的字体，避免使用缩放
            auto defaultFont = fontManager->getDefaultFont();
            if (defaultFont) {
                defaultFont->pushFont();
                usedFont = defaultFont;
            }
        }
        
        // 使用较大的字体大小显示时间文本
        textSize = ImGui::CalcTextSize(timeText.c_str());
        ImGui::SetCursorPosX((availableSize.x - textSize.x) * 0.5f);
        ImGui::SetCursorPosY(50);
        ImGui::SetWindowFontScale(2.0f); // 使用适中的缩放以保持清晰度
        ImGui::Text("%s", timeText.c_str());
        ImGui::SetWindowFontScale(1.0f); // 立即恢复默认缩放
        
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
        ImGui::SetWindowFontScale(1.1f); // 适度放大标题文本
        ImGui::Text("设置时间:");
        ImGui::SetWindowFontScale(1.0f);
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
        DEARTS_LOG_INFO("番茄时钟开始计时");
        
        // 显示开始通知
        if (isWorkMode_) {
          showNotification("番茄时钟", "开始工作时间！");
        } else {
          showNotification("番茄时钟", "开始休息时间！");
        }
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
        DEARTS_LOG_INFO("番茄时钟切换模式: " + currentModeText_);
      }

      /**
       * 显示Windows通知
       */
      void PomodoroLayout::showNotification(const std::string& title, const std::string& message) {
#ifdef _WIN32
        // 使用线程来显示通知，避免阻塞UI
        std::thread([title, message]() {
            // 初始化COM
            HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(hr)) {
                try {
                    // 使用Windows Runtime API创建Toast通知
                    using namespace winrt;
                    using namespace Windows::UI::Notifications;
                    using namespace Windows::Data::Xml::Dom;
                    
                    // 创建Toast通知XML内容
                    std::wstring xml = L"<toast>"
                                       L"<visual>"
                                       L"<binding template=\"ToastGeneric\">"
                                       L"<text>" + std::wstring(title.begin(), title.end()) + L"</text>"
                                       L"<text>" + std::wstring(message.begin(), message.end()) + L"</text>"
                                       L"</binding>"
                                       L"</visual>"
                                       L"</toast>";
                    
                    // 加载XML
                    XmlDocument doc;
                    doc.LoadXml(xml);
                    
                    // 创建Toast通知
                    ToastNotification toast(doc);
                    
                    // 获取Toast通知管理器
                    ToastNotificationManager::CreateToastNotifier().Show(toast);
                } catch (...) {
                    // 如果Toast通知失败，使用传统方式
                    std::wstring wTitle(title.begin(), title.end());
                    std::wstring wMessage(message.begin(), message.end());
                    MessageBoxW(NULL, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONINFORMATION);
                }
                
                CoUninitialize();
            } else {
                // 如果COM初始化失败，使用传统方式
                std::wstring wTitle(title.begin(), title.end());
                std::wstring wMessage(message.begin(), message.end());
                MessageBoxW(NULL, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONINFORMATION);
            }
        }).detach();
#endif
      }

      /**
       * 更新计时器状态
       */
      void PomodoroLayout::updateTimer() {
        static std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();

        // 只有在可见且运行时才更新计时器
        if (isVisible_ && isRunning_) {
          // 使用更精确的时间计算
          static double accumulatedTime = 0.0;
          accumulatedTime += deltaTime;
          
          // 每秒更新一次
          if (accumulatedTime >= 1.0) {
            int secondsToSubtract = (int)accumulatedTime;
            remainingTime_ -= secondsToSubtract;
            accumulatedTime -= secondsToSubtract;
            
            // 记录倒计时更新
            DEARTS_LOG_INFO("番茄时钟倒计时更新，剩余时间: " + std::to_string(remainingTime_) + "秒");
            
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

        lastTime = currentTime;
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