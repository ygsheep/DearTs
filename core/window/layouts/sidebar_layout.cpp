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
          activeItemId_(""), backgroundColor_(0.15f, 0.15f, 0.15f, 1.0f), itemNormalColor_(0.2f, 0.2f, 0.2f, 1.0f),
          itemHoverColor_(0.3f, 0.3f, 0.3f, 1.0f), itemActiveColor_(0.0f, 0.5f, 1.0f, 1.0f),
          itemTextColor_(0.8f, 0.8f, 0.8f, 1.0f), itemTextHoverColor_(1.0f, 1.0f, 1.0f, 1.0f) {
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
          isExpanded_ = expanded;
          targetWidth_ = isExpanded_ ? sidebarWidth_ : collapsedWidth_;
          isAnimating_ = true;
          // 在没有ImGui上下文的情况下使用当前时间
          animationStartTime_ = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                       std::chrono::high_resolution_clock::now().time_since_epoch())
                                                       .count());

          // 立即触发状态变化回调，报告目标宽度
          if (stateCallback_) {
            stateCallback_(isExpanded_, targetWidth_);
          }
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

            // 动画完成时触发回调
            triggerStateCallback();
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

        // 触发项目点击回调
        if (itemClickCallback_) {
          itemClickCallback_(itemId);
        }

        // 可以在这里添加更多的点击处理逻辑
        // 例如触发事件或回调函数
        // DEARTS_LOG_INFO("Sidebar item clicked: " + itemId);
        // 在测试环境中可能没有初始化日志系统，所以暂时注释掉
      }

      /**
       * 触发状态变化回调
       */
      void SidebarLayout::triggerStateCallback() {
        if (stateCallback_) {
          stateCallback_(isExpanded_, currentWidth_);
        }
      }

    } // namespace Window
  } // namespace Core
} // namespace DearTs
