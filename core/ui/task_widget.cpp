/**
 * @file task_widget.cpp
 * @brief 任务管理器 UI 组件实现
 */

#include "task_widget.h"
#include "logger.h"
#include <cmath>

namespace DearTs::Core::UI {

void TaskWidget::render() {
    auto& task_manager = Tasks::TaskManager::instance();
    auto running_tasks = task_manager.getRunningTasks();

    if (running_tasks.empty()) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Tasks")) {
        ImGui::Text("Running Tasks: %zu", running_tasks.size());
        ImGui::Separator();

        for (const auto& task : running_tasks) {
            renderTask(task);
            ImGui::Separator();
        }
    }
    ImGui::End();
}

void TaskWidget::renderTask(const std::shared_ptr<Tasks::Task>& task) {
    if (!task) {
        return;
    }

    // 任务名称和类型
    ImGui::Text("%s (%s)", task->getName().c_str(), getTypeText(task->getType()));

    // 状态标签
    ImGui::SameLine();
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::PushStyleColor(ImGuiCol_Text, getStatusColor(task->getStatus()));
    ImGui::Text("[%s]", getStatusText(task->getStatus()));
    ImGui::PopStyleColor();

    // 进度条
    float progress = task->getProgressPercent();
    char progress_text[64];
    snprintf(progress_text, sizeof(progress_text), "%.0f%%", progress * 100);

    ImGui::ProgressBar(progress, ImVec2(-1, 0), progress_text);

    // 取消按钮
    if (task->isRunning()) {
        if (ImGui::Button("Cancel")) {
            Tasks::TaskManager::instance().cancel(task);
        }
    }
}

void TaskWidget::renderCompact() {
    auto& task_manager = Tasks::TaskManager::instance();
    size_t running_count = task_manager.getRunningTaskCount();

    if (running_count == 0) {
        return;
    }

    // 紧凑显示：在状态栏显示 "Tasks: 3" 样式
    char text[64];
    snprintf(text, sizeof(text), "Tasks: %zu", running_count);

    ImVec2 size = ImGui::CalcTextSize(text);
    ImGui::SameLine(ImGui::GetWindowWidth() - size.x - 10);

    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", text);

    // 显示进度
    auto running_tasks = task_manager.getRunningTasks();
    if (!running_tasks.empty()) {
        auto& task = running_tasks[0];
        float progress = task->getProgressPercent();
        ImGui::SameLine(0, 10);
        ImGui::Text("[%.0f%%]", progress * 100);
    }
}

const char* TaskWidget::getStatusText(Tasks::TaskStatus status) {
    switch (status) {
        case Tasks::TaskStatus::Pending:   return "等待中";
        case Tasks::TaskStatus::Running:   return "运行中";
        case Tasks::TaskStatus::Completed: return "已完成";
        case Tasks::TaskStatus::Cancelled: return "已取消";
        case Tasks::TaskStatus::Failed:    return "失败";
        default:                           return "未知";
    }
}

const char* TaskWidget::getTypeText(Tasks::TaskType type) {
    switch (type) {
        case Tasks::TaskType::Normal:     return "普通";
        case Tasks::TaskType::Background: return "后台";
        case Tasks::TaskType::Blocking:   return "阻塞";
        case Tasks::TaskType::Critical:   return "关键";
        default:                          return "未知";
    }
}

ImVec4 TaskWidget::getStatusColor(Tasks::TaskStatus status) {
    switch (status) {
        case Tasks::TaskStatus::Pending:   return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        case Tasks::TaskStatus::Running:   return ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
        case Tasks::TaskStatus::Completed: return ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
        case Tasks::TaskStatus::Cancelled: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        case Tasks::TaskStatus::Failed:    return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        default:                           return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

} // namespace DearTs::Core::UI
