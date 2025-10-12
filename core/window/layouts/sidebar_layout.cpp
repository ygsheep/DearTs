#include "sidebar_layout.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include "../resource/font_resource.h"
#include "../utils/logger.h"
#include "../window_base.h"
#include "title_bar_layout.h"

namespace DearTs {
  namespace Core {
    namespace Window {

      /**
       * SidebarLayout构造函数
       */
      SidebarLayout::SidebarLayout() :
          LayoutBase("Sidebar"), isExpanded_(true), isAnimating_(false), currentWidth_(180.0f), targetWidth_(180.0f),
          sidebarWidth_(180.0f), collapsedWidth_(48.0f), animationDuration_(300.0f), animationStartTime_(0.0f),
          activeItemId_(""), backgroundColor_(0.20f, 0.20f, 0.20f, 1.0f), itemNormalColor_(0.2f, 0.2f, 0.2f, 1.0f),
          itemHoverColor_(0.3f, 0.3f, 0.3f, 1.0f), itemActiveColor_(0.0f, 0.5f, 1.0f, 1.0f),
          itemTextColor_(0.8f, 0.8f, 0.8f, 1.0f), itemTextHoverColor_(1.0f, 1.0f, 1.0f, 1.0f),
          currentState_(SidebarState::EXPANDED) {
        currentWidth_ = isExpanded_ ? sidebarWidth_ : collapsedWidth_;
        targetWidth_ = isExpanded_ ? sidebarWidth_ : collapsedWidth_;
      }

      /**
       * 渲染侧边栏布局
       */
      void SidebarLayout::render() {
        // 获取标题栏高度
        float titleBarHeight = 30.0f; // 默认高度
        if (parentWindow_) {
          // 尝试获取标题栏布局并获取其高度
          auto *titleBarLayout = parentWindow_->getLayout("TitleBar");
          if (titleBarLayout) {
            // 检查是否为TitleBarLayout类型并获取高度
            auto *titleBar = dynamic_cast<TitleBarLayout *>(titleBarLayout);
            if (titleBar) {
              titleBarHeight = titleBar->getTitleBarHeight();
            }
          }
        }

        // 设置侧边栏位置和大小（从标题栏下方开始，高度减去标题栏高度）
        ImGui::SetNextWindowPos(ImVec2(0, titleBarHeight));
        ImGui::SetNextWindowSize(ImVec2(currentWidth_, ImGui::GetIO().DisplaySize.y - titleBarHeight));

        // 创建侧边栏窗口
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, backgroundColor_);
        ImGui::Begin("Sidebar", nullptr, flags);

        // 根据当前状态渲染侧边栏
        if (isExpanded_ || (isAnimating_ && targetWidth_ > collapsedWidth_)) {
          renderExpanded();
        } else {
          renderCollapsed();
        }

        ImGui::End();
        ImGui::PopStyleColor();
      }

      /**
       * 更新侧边栏布局
       */
      void SidebarLayout::updateLayout(float width, float height) {
        // 更新动画状态
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        updateAnimation(deltaTime);

        // 获取标题栏高度
        float titleBarHeight = 30.0f; // 默认高度
        if (parentWindow_) {
          // 尝试获取标题栏布局并获取其高度
          auto *titleBarLayout = parentWindow_->getLayout("TitleBar");
          if (titleBarLayout) {
            // 检查是否为TitleBarLayout类型并获取高度
            auto *titleBar = dynamic_cast<TitleBarLayout *>(titleBarLayout);
            if (titleBar) {
              titleBarHeight = titleBar->getTitleBarHeight();
            }
          }
        }

        // 更新位置和大小（高度减去标题栏高度）
        setPosition(0, titleBarHeight);
        setSize(currentWidth_, height - titleBarHeight);
      }

      /**
       * 处理侧边栏事件
       */
      void SidebarLayout::handleEvent(const SDL_Event &event) {
        // 将SDL事件传递给ImGui，确保ImGui能正确处理鼠标和键盘输入
        ImGui_ImplSDL2_ProcessEvent(&event);

        // 在这里可以添加额外的事件处理逻辑
        // 例如处理特定的鼠标悬停、点击等事件
      }

      /**
       * 设置侧边栏展开状态
       */
      void SidebarLayout::setExpanded(bool expanded) {
        if (isExpanded_ != expanded) {
          // 更新状态
          SidebarState oldState = currentState_;
          currentState_ = expanded ? SidebarState::EXPANDING : SidebarState::COLLAPSING;

          isExpanded_ = expanded;
          targetWidth_ = isExpanded_ ? sidebarWidth_ : collapsedWidth_;
          isAnimating_ = true;

          // 在没有ImGui上下文的情况下使用当前时间
          animationStartTime_ = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                       std::chrono::high_resolution_clock::now().time_since_epoch())
                                                       .count());

          // 发送状态变化事件
          SidebarEventData stateEvent(SidebarEventType::STATE_CHANGED, "", expanded);
          dispatchSidebarEvent(stateEvent);

          // 发送动画开始事件
          SidebarEventData animEvent(SidebarEventType::ANIMATION_STARTED, "", targetWidth_);
          dispatchSidebarEvent(animEvent);

          // 立即触发状态变化回调，报告目标宽度
          if (stateCallback_) {
            stateCallback_(isExpanded_, targetWidth_);
          }

          DEARTS_LOG_DEBUG("侧边栏状态变化: " + std::to_string(static_cast<int>(oldState)) +
                           " -> " + std::to_string(static_cast<int>(currentState_)));
        }
      }

      /**
       * 切换侧边栏展开状态
       */
      void SidebarLayout::toggleExpanded() { setExpanded(!isExpanded_); }

      /**
       * 添加侧边栏项目
       */
      void SidebarLayout::addItem(const SidebarItem &item) {
        // 检查是否已存在相同ID的项目
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&item](const SidebarItem &existingItem) { return existingItem.id == item.id; });

        if (it == items_.end()) {
          items_.push_back(item);
        }
      }

      /**
       * 移除侧边栏项目
       */
      void SidebarLayout::removeItem(const std::string &id) {
        items_.erase(
            std::remove_if(items_.begin(), items_.end(), [&id](const SidebarItem &item) { return item.id == id; }),
            items_.end());
      }

      /**
       * 获取侧边栏项目
       */
      SidebarItem *SidebarLayout::getItem(const std::string &id) {
        auto it = std::find_if(items_.begin(), items_.end(), [&id](const SidebarItem &item) { return item.id == id; });

        return (it != items_.end()) ? &(*it) : nullptr;
      }

      /**
       * 清除所有项目
       */
      void SidebarLayout::clearItems() { items_.clear(); }

      /**
       * 设置当前激活项目
       */
      void SidebarLayout::setActiveItem(const std::string &id) {
        // 取消之前激活项目的激活状态
        if (!activeItemId_.empty()) {
          SidebarItem *previousItem = getItem(activeItemId_);
          if (previousItem) {
            previousItem->isActive = false;
          }
        }

        // 设置新激活项目
        SidebarItem *newItem = getItem(id);
        if (newItem) {
          newItem->isActive = true;
          activeItemId_ = id;
        }
      }

      /**
       * 更新动画状态
       */
      void SidebarLayout::updateAnimation(double deltaTime) {
        if (isAnimating_) {
          float currentTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                     std::chrono::high_resolution_clock::now().time_since_epoch())
                                                     .count());
          float elapsed = currentTime - animationStartTime_;
          float progress = std::min(elapsed / animationDuration_, 1.0f);

          // 使用缓动函数使动画更流畅
          float easedProgress = easeOutCubic(progress);

          // 插值计算当前宽度
          currentWidth_ = currentWidth_ + (targetWidth_ - currentWidth_) * easedProgress;

          // 检查动画是否完成
          if (progress >= 1.0f) {
            isAnimating_ = false;
            currentWidth_ = targetWidth_;

            // 更新最终状态
            currentState_ = isExpanded_ ? SidebarState::EXPANDED : SidebarState::COLLAPSED;

            // 发送动画完成事件
            SidebarEventData animCompletedEvent(SidebarEventType::ANIMATION_COMPLETED, "", currentWidth_);
            dispatchSidebarEvent(animCompletedEvent);

            // 发送最终状态事件
            SidebarEventType finalEventType = isExpanded_ ? SidebarEventType::EXPANDED : SidebarEventType::COLLAPSED;
            SidebarEventData finalEvent(finalEventType, "", currentWidth_);
            dispatchSidebarEvent(finalEvent);

            // 动画完成时触发回调
            triggerStateCallback();

            DEARTS_LOG_DEBUG("侧边栏动画完成，最终状态: " + std::to_string(static_cast<int>(currentState_)));
          }

          // 如果正在动画，也触发回调以通知宽度变化
          if (isAnimating_) {
            triggerStateCallback();
          }
        }
      }

      /**
       * 渲染展开状态的侧边栏
       */
      void SidebarLayout::renderExpanded() {
        // 渲染项目列表（设置上边距为20像素）
        ImGui::SetCursorPos(ImVec2(0, 20));

        for (auto &item: items_) {
          renderItem(item, true);
        }
      }

      /**
       * 渲染收起状态的侧边栏
       */
      void SidebarLayout::renderCollapsed() {
        // 渲染项目图标（设置上边距为20像素）
        ImGui::SetCursorPos(ImVec2(0, 20));

        for (auto &item: items_) {
          renderItem(item, false);
        }
      }

      /**
       * 渲染侧边栏项目
       */
      void SidebarLayout::renderItem(const SidebarItem &item, bool isExpanded) {
        float itemHeight = 20.0f;

        // 使用ImGui的折叠菜单组件
        ImGui::SetCursorPosX(10.0f);
        std::string treeNodeId = "##" + item.id;

        // 设置树节点的开放状态
        ImGui::SetNextItemOpen(item.isExpanded, ImGuiCond_Always);

        // 创建树节点
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding |
                                   ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bool nodeOpen = ImGui::TreeNodeEx(treeNodeId.c_str(), flags, "%s", item.text.c_str());

        // 处理点击事件（点击箭头或文本都可以切换展开状态）
        if (ImGui::IsItemClicked()) {
          // 切换展开状态
          SidebarItem *mutableItem = const_cast<SidebarItem *>(&item);
          mutableItem->isExpanded = !item.isExpanded;
        }

        // 如果节点打开，渲染子项目
        if (nodeOpen || item.isExpanded) {
          // 增加缩进
          ImGui::Indent(10.0f);

          // 渲染子项目（不使用图片）
          for (const auto &child: item.children) {
            // 获取当前光标位置
            ImVec2 cursorPos = ImGui::GetCursorPos();

            // 绘制文本（移除问号帮助提示）
            ImGui::SameLine();
            ImGui::SetCursorPos(
                ImVec2(cursorPos.x + 20.0, cursorPos.y + (itemHeight - ImGui::GetTextLineHeight()) / 2.0f + 2.0f));
            ImGui::Text("%s", child.text.c_str());

            // 处理子项目点击事件和悬停效果（放在最后以确保正确的层级关系）
            ImGui::SetCursorPos(cursorPos);
            ImGui::InvisibleButton(("##child_" + child.id).c_str(), ImVec2(currentWidth_ - 40, itemHeight));

            // 处理子项目点击事件
            if (ImGui::IsItemClicked()) {
              handleItemClick(child.id);
            }
          }
        }

        // 减少缩进
        ImGui::Unindent(10.0f);
      }

      /**
       * 计算动画插值（缓出立方）
       */
      float SidebarLayout::easeOutCubic(float t) const { return 1.0f - powf(1.0f - t, 3.0f); }

      /**
       * 处理项目点击事件
       */
      void SidebarLayout::handleItemClick(const std::string &itemId) {
        setActiveItem(itemId);

        // 发送项目点击事件
        SidebarEventData clickEvent(SidebarEventType::ITEM_CLICKED, itemId, itemId);
        dispatchSidebarEvent(clickEvent);

        // 使用事件驱动机制请求布局切换
        requestLayoutSwitch(itemId);

        // 保持向后兼容性 - 触发传统回调
        if (itemClickCallback_) {
          itemClickCallback_(itemId);
        }

        DEARTS_LOG_INFO("侧边栏项目点击: " + itemId);
      }

      /**
       * 触发状态变化回调
       */
      void SidebarLayout::triggerStateCallback() {
        if (stateCallback_) {
          stateCallback_(isExpanded_, currentWidth_);
        }
      }

      // === 事件驱动方法实现 ===

      void SidebarLayout::dispatchSidebarEvent(const SidebarEventData& eventData) {
        // 添加到事件历史记录
        eventHistory_.push_back(eventData);

        // 限制历史记录数量
        if (eventHistory_.size() > 100) {
          eventHistory_.erase(eventHistory_.begin());
        }

        // 调用事件回调
        if (eventCallback_) {
          eventCallback_(eventData);
        }

        // 记录日志（使用C++17结构化绑定）
        const auto& [type, itemId, timestamp, data] = eventData;
        DEARTS_LOG_DEBUG("侧边栏事件: 类型=" + std::to_string(static_cast<int>(type)) +
                         ", 项目ID=" + itemId);
      }

      void SidebarLayout::initializeEventSystem() {
        DEARTS_LOG_INFO("侧边栏事件系统初始化");

        // 初始化事件历史记录
        eventHistory_.clear();

        // 发送初始化事件
        dispatchSidebarEvent(SidebarEventData(SidebarEventType::STATE_CHANGED, "",
                                               static_cast<bool>(currentState_ == SidebarState::EXPANDED)));

        DEARTS_LOG_INFO("侧边栏事件系统初始化完成");
      }

      void SidebarLayout::cleanupEventSystem() {
        DEARTS_LOG_INFO("侧边栏事件系统清理");

        // 清空事件历史记录
        eventHistory_.clear();

        // 清空事件回调
        eventCallback_ = nullptr;

        DEARTS_LOG_INFO("侧边栏事件系统清理完成");
      }

      void SidebarLayout::subscribeSidebarEvent(Events::EventType eventType,
                                              std::function<bool(const Events::Event&)> handler) {
        // 通过父窗口订阅事件
        if (parentWindow_) {
          parentWindow_->subscribeEvent(eventType, handler);
          DEARTS_LOG_DEBUG("侧边栏订阅事件: " + std::to_string(static_cast<uint32_t>(eventType)));
        }
      }

      void SidebarLayout::requestLayoutSwitch(const std::string& itemId) {
        DEARTS_LOG_INFO("侧边栏请求布局切换: " + itemId);

        // 创建布局切换事件类
        class LayoutSwitchEvent : public Events::Event {
        public:
          LayoutSwitchEvent(const std::string& from, const std::string& to, bool animated)
              : Event(Events::EventType::EVT_LAYOUT_SWITCH_REQUEST),
                fromLayout_(from), toLayout_(to), animated_(animated) {}

          std::string getName() const override { return "LayoutSwitchEvent"; }

          std::string getFromLayout() const { return fromLayout_; }
          std::string getToLayout() const { return toLayout_; }
          bool isAnimated() const { return animated_; }

        private:
          std::string fromLayout_;
          std::string toLayout_;
          bool animated_;
        };

        // 发送布局切换事件
        LayoutSwitchEvent switchEvent(getActiveItemId(), itemId, true);
        if (parentWindow_) {
          parentWindow_->dispatchWindowEvent(switchEvent);
        }

        // 发送侧边栏内部事件
        dispatchSidebarEvent(SidebarEventData(SidebarEventType::ITEM_CLICKED, itemId, itemId));
      }

    } // namespace Window
  } // namespace Core
} // namespace DearTs
