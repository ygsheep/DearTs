/**
 * @file ffmpeg_view.cpp
 * @brief FFmpeg 视图实现
 */

#include "ffmpeg_view.hpp"
#include "liblogger/logger.h"
#include "core/utils/file_dialog.hpp"
#include "core/ui/theme_manager.h"

#ifdef _WIN32
#include <windows.h>
#endif

#if DEARTS_FFMPEG_SUPPORT

// 注意：ToastManager 只在实现文件中包含，避免循环依赖
#include "toast_manager.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <format>

namespace DearTs::Plugins::FFmpeg {

// ============================================================================
// 构造/析构
// ============================================================================

FFmpegView::FFmpegView() : ViewWindow(Core::UI::UnlocalizedString("合并TS文件")) {
    // 设置默认打开
    m_window_open = true;

    LOG_INFO("FFmpegView: Created");

    // 检测硬件
    m_hardware_info = detect_hardware();
    m_hardware_detected = true;

    // 设置默认编码器
    if (m_hardware_info.recommended_encoder != EncoderType::CPU) {
        m_selected_encoder = m_hardware_info.recommended_encoder;
    }

    // 输出硬件信息
    LOG_INFO("FFmpegView: Hardware detection completed");
    LOG_INFO("  NVIDIA GPU: {}", m_hardware_info.has_nvidia_gpu ? m_hardware_info.nvidia_gpu_name : "None");
    LOG_INFO("  AMD GPU: {}", m_hardware_info.has_amd_gpu ? m_hardware_info.amd_gpu_name : "None");
    LOG_INFO("  Intel GPU: {}", m_hardware_info.has_intel_gpu ? m_hardware_info.intel_gpu_name : "None");
    LOG_INFO("  Recommended encoder: {}", static_cast<int>(m_hardware_info.recommended_encoder));

    // 订阅任务事件以显示 Toast 通知
    using namespace Core::Event;
    using namespace Core::Tasks;

    m_task_completed_token = EventBus::instance().subscribe<TaskCompletedEvent>(
        [&](const TaskCompletedEvent& event) {
            LOG_INFO("FFmpegView: TaskCompletedEvent received, task: {}", event.task->getName());

            // 检查是否是我们的合并任务
            bool is_merge_task = (m_merge_task && event.task.get() == m_merge_task.get());

            if (is_merge_task) {
                LOG_INFO("FFmpegView: Merge task completed");
                m_merge_progress.active = false;
                m_merge_progress.progress = 1.0;
            } else {
                // 其他任务也显示通知（用于调试）
                LOG_DEBUG("FFmpegView: Other task completed: {}", event.task->getName());
            }
        }
    );

    m_task_failed_token = EventBus::instance().subscribe<TaskFailedEvent>(
        [&](const TaskFailedEvent& event) {
            LOG_INFO("FFmpegView: TaskFailedEvent received, task: {}, error: {}",
                     event.task->getName(), event.error_message);

            auto& toast = Toast::ToastManager::instance();

            // 检查是否是我们的合并任务
            bool is_merge_task = (m_merge_task && event.task.get() == m_merge_task.get());

            if (is_merge_task) {
                LOG_INFO("FFmpegView: Merge task failed");
                toast.error("合并失败", event.error_message);
                m_merge_progress.active = false;
                m_merge_progress.error_message = event.error_message;
            } else {
                // 其他任务也显示通知（用于调试）
                LOG_DEBUG("FFmpegView: Other task failed: {}", event.task->getName());
            }
        }
    );

    m_task_cancelled_token = EventBus::instance().subscribe<TaskCancelledEvent>(
        [&](const TaskCancelledEvent& event) {
            LOG_INFO("FFmpegView: TaskCancelledEvent received, task: {}", event.task->getName());

            auto& toast = Toast::ToastManager::instance();

            // 检查是否是我们的合并任务
            bool is_merge_task = (m_merge_task && event.task.get() == m_merge_task.get());

            if (is_merge_task) {
                LOG_INFO("FFmpegView: Merge task cancelled");
                toast.warning("合并取消", std::format("耗时: {:.1f}秒", event.duration_ms / 1000.0));
                m_merge_progress.active = false;
                m_merge_progress.current_action = "Cancelled";
            } else {
                // 其他任务也显示通知（用于调试）
                LOG_DEBUG("FFmpegView: Other task cancelled: {}", event.task->getName());
            }
        }
    );
}

FFmpegView::~FFmpegView() {
}

// ============================================================================
// 主绘制方法
// ============================================================================

void FFmpegView::draw_content() {

    draw_header();

    ImGui::Spacing();

    draw_file_list();

    ImGui::Spacing();

    draw_control_buttons();
}

// ============================================================================
// 头部区域
// ============================================================================

void FFmpegView::draw_header() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 0.6f));

    if (ImGui::BeginChild("Header", ImVec2(0, 60), true)) {
        // 计算垂直居中位置
        float header_height = 60.0f;
        float text_height = ImGui::GetTextLineHeight();
        float vertical_offset = (header_height - text_height) * 0.5f;

        // 左侧：标题和文件计数
        ImGui::SetCursorPosY(vertical_offset);

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // 大字体
        ImGui::TextColored(Core::UI::ThemeManager::instance().getAccentColor(), "FFmpeg 视频工具");
        ImGui::PopFont();

        ImGui::SameLine();

        // 文件计数（也需要垂直对齐）
        float title_height = ImGui::GetTextLineHeight() * 1.5f; // 大字体高度
        float label_offset = (header_height - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPosY(label_offset);
        ImGui::TextDisabled("%zu 个文件", m_files.size());

        // 右侧：编码器选择
        ImGui::SameLine(ImGui::GetWindowWidth() - 220);

        // 编码器选择器（也垂直居中）
        ImGui::SetCursorPosY(label_offset);
        ImGui::Text("编码器:");
        ImGui::SameLine();

        int current_encoder = static_cast<int>(m_selected_encoder);

        // 构建编码器选项列表
        const char* encoder_items[] = {
            "自动 (推荐)",
            "CPU (libx264)",
            "NVIDIA NVENC",
            "AMD AMF",
            "Intel QSV"
        };

        // 根据硬件检测情况启用/禁用选项
        static const char* encoder_items_disabled[] = {
            "自动 (推荐)",
            "CPU (libx264)",
            "NVIDIA NVENC ✗",
            "AMD AMF ✗",
            "Intel QSV ✗"
        };

        // 确定当前选中的索引
        int encoder_index = 0;
        switch (m_selected_encoder) {
            case EncoderType::Auto: encoder_index = 0; break;
            case EncoderType::CPU: encoder_index = 1; break;
            case EncoderType::NVIDIA_NVENC: encoder_index = 2; break;
            case EncoderType::AMD_VCE: encoder_index = 3; break;
            case EncoderType::Intel_QSV: encoder_index = 4; break;
        }

        // 使用下拉选择器
        int prev_index = encoder_index;
        ImGui::SetNextItemWidth(140);

        // 根据硬件状态决定是否禁用某些选项
        bool has_gpu = m_hardware_detected &&
                       (m_hardware_info.has_nvidia_gpu ||
                        m_hardware_info.has_amd_gpu ||
                        m_hardware_info.has_intel_gpu);

        if (ImGui::BeginCombo("##encoder", has_gpu ? encoder_items[encoder_index] : encoder_items_disabled[encoder_index])) {
            for (int i = 0; i < 5; ++i) {
                // 检查该选项是否可用
                bool available = true;
                if (i == 2 && !m_hardware_info.has_nvidia_gpu) available = false;
                if (i == 3 && !m_hardware_info.has_amd_gpu) available = false;
                if (i == 4 && !m_hardware_info.has_intel_gpu) available = false;

                if (!available && i > 1) {
                    // 显示但禁用
                    ImGui::BeginDisabled();
                    ImGui::Selectable(encoder_items_disabled[i], false);
                    ImGui::EndDisabled();
                } else {
                    bool is_selected = (encoder_index == i);
                    if (ImGui::Selectable(encoder_items[i], is_selected)) {
                        encoder_index = i;
                        switch (i) {
                            case 0: m_selected_encoder = EncoderType::Auto; break;
                            case 1: m_selected_encoder = EncoderType::CPU; break;
                            case 2: m_selected_encoder = EncoderType::NVIDIA_NVENC; break;
                            case 3: m_selected_encoder = EncoderType::AMD_VCE; break;
                            case 4: m_selected_encoder = EncoderType::Intel_QSV; break;
                        }
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ============================================================================
// 文件列表
// ============================================================================

void FFmpegView::draw_file_list() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 0.5f));

    // 使用固定的窗口高度来计算列表高度
    float window_height = ImGui::GetWindowHeight();
    float header_height = 60;   // 头部高度
    float buttons_height = 50;  // 按钮区域高度
    float spacing = 40;          // 间距

    // 进度条高度（如果显示）
    float progress_height = m_merge_progress.active.load() ? 80 : 0;

    // 计算文件列表的固定高度
    float list_height = window_height - header_height - buttons_height - spacing - progress_height - 20;

    // 确保高度不为负
    if (list_height < 100) list_height = 100;

    if (ImGui::BeginChild("FileList", ImVec2(0, list_height), true)) {
        if (m_files.empty()) {
            // 空状态
            ImVec2 center = ImGui::GetCursorScreenPos();
            center.x += ImGui::GetContentRegionAvail().x * 0.5f;
            center.y += ImGui::GetContentRegionAvail().y * 0.4f;

            ImGui::SetCursorScreenPos(center);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
            ImGui::TextWrapped("拖放 .ts 文件到此处或点击'添加文件'开始");
            ImGui::PopStyleColor();
        } else {
            // 文件列表头部：全选/取消全选切换按钮
            size_t selected_count = m_selected_files_set.size();
            bool all_selected = (selected_count == m_files.size()) && !m_files.empty();

            // 切换全选按钮
            if (ImGui::Button(all_selected ? "取消全选" : "全选")) {
                if (all_selected) {
                    deselect_all_files();
                } else {
                    select_all_files();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("删除选中")) {
                remove_selected_files();
            }

            // 显示已选数量
            if (selected_count > 0) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "已选择 %zu / %zu 个文件", selected_count, m_files.size());
            }

            ImGui::Separator();
            ImGui::Spacing();

            // 文件列表
            for (size_t i = 0; i < m_files.size(); ++i) {
                const auto& file = m_files[i];
                ImGui::PushID(i);

                // 选中高亮状态
                bool is_selected = (m_selected_file == i);

                // 使用 Selectable 作为主行背景（不使用 SpanAllColumns，避免挡住复选框）
                ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                ImVec2 item_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 2 + ImGui::GetStyle().ItemSpacing.y * 2);

                // 绘制选中背景
                if (is_selected) {
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(
                        cursor_pos,
                        ImVec2(cursor_pos.x + item_size.x, cursor_pos.y + item_size.y),
                        ImGui::GetColorU32(Core::UI::ThemeManager::instance().getAccentColor()),
                        ImGui::GetStyle().FrameRounding
                    );
                }

                // 复选框
                bool is_checked = is_file_selected(i);
                ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x + 5, cursor_pos.y + item_size.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f));
                if (ImGui::Checkbox("##select", &is_checked)) {
                    toggle_file_selection(i);
                }

                // 文件信息
                ImGui::SameLine(0, 10);

                ImGui::BeginGroup();
                if (file.valid) {
                    ImGui::Text("%s", file.name.c_str());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s (无效)", file.name.c_str());
                }

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
                ImGui::Text("%s | %s",
                    format_file_size(file.size).c_str(),
                    format_duration(file.duration).c_str());
                ImGui::PopStyleColor();
                ImGui::EndGroup();

                // 可点击区域（用于选择和高亮）
                ImVec2 clickable_min = cursor_pos;
                ImVec2 clickable_max = ImVec2(cursor_pos.x + item_size.x, cursor_pos.y + item_size.y);
                ImGui::SetCursorScreenPos(cursor_pos);
                ImGui::InvisibleButton("##item", item_size);

                // 处理点击
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    m_selected_file = i;
                }

                // 双击处理
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    // 可以添加双击功能
                }

                // 右键菜单
                if (ImGui::BeginPopupContextItem("##context")) {
                    if (ImGui::Selectable("上移")) {
                        move_file_up(i);
                    }
                    if (ImGui::Selectable("下移")) {
                        move_file_down(i);
                    }
                    ImGui::Separator();
                    if (ImGui::Selectable("删除")) {
                        remove_file(i);
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // 如果正在合并，显示进度条
    if (m_merge_progress.active.load()) {
        ImGui::Spacing();

        // 进度条容器 - 固定高度
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 0.5f));
        if (ImGui::BeginChild("ProgressBar", ImVec2(0, 70), true)) {
            // 当前动作
            ImGui::Text("%s", m_merge_progress.current_action.c_str());

            // 进度条
            float progress = m_merge_progress.progress.load();
            ImGui::ProgressBar(progress, ImVec2(-1, 0),
                std::format("{:.1f}%", progress * 100).c_str());

            // 取消按钮
            ImGui::Spacing();

            float cancel_width = 100;
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - cancel_width);

            if (ImGui::Button("取消", ImVec2(cancel_width, 0))) {
                cancel_merge();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

// ============================================================================
// 控制按钮
// ============================================================================

void FFmpegView::draw_control_buttons() {
    float button_width = 140;
    float spacing = 10;

    // 计算总宽度并居中
    int button_count = 4;
    float total_width = button_width * button_count + spacing * (button_count - 1);
    float start_x = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

    ImGui::SetCursorPosX(start_x);

    // 添加文件
    if (ImGui::Button("添加文件", ImVec2(button_width, 0))) {
        add_files();
    }

    ImGui::SameLine(0, spacing);

    // 添加文件夹
    if (ImGui::Button("添加文件夹", ImVec2(button_width, 0))) {
        add_folder();
    }

    ImGui::SameLine(0, spacing);

    // 清空列表
    if (ImGui::Button("清空全部", ImVec2(button_width, 0))) {
        clear_files();
    }

    ImGui::SameLine(0, spacing);

    // 开始合并
    bool can_merge = !m_files.empty() &&
                     std::all_of(m_files.begin(), m_files.end(),
                                  [](const TSFile& f) { return f.valid; });

    if (!can_merge) {
        ImGui::BeginDisabled();
    }

    ImVec4 merge_color = ImVec4(0.2f, 0.7f, 0.3f, 1.0f); // 绿色
    ImGui::PushStyleColor(ImGuiCol_Button, merge_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));

    if (ImGui::Button("合并TS", ImVec2(button_width, 0))) {
        start_merge();
    }

    ImGui::PopStyleColor(2);

    if (!can_merge) {
        ImGui::EndDisabled();
    }

    // 文件选择对话框
    if (m_show_file_selector) {
        draw_file_selector();
    }
}

// ============================================================================
// 进度对话框
// ============================================================================

void FFmpegView::draw_progress_dialog() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 0));

    if (ImGui::BeginPopupModal("合并进度", &m_show_progress,
                                 ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("正在合并TS文件...");
        ImGui::Separator();

        // 进度条
        float progress = m_merge_progress.progress.load();
        ImGui::ProgressBar(progress, ImVec2(-1, 0),
                           m_merge_progress.current_action.c_str());

        ImGui::Spacing();

        // 详细信息
        int current = m_merge_progress.current_file.load();
        int total = m_merge_progress.total_files.load();

        ImGui::Text("处理中: %d / %d", current, total);

        // 错误信息
        if (!m_merge_progress.error_message.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "错误: %s",
                               m_merge_progress.error_message.c_str());
        }

        ImGui::Spacing();

        // 完成或取消按钮
        if (progress >= 1.0f || !m_merge_progress.error_message.empty()) {
            if (ImGui::Button("关闭", ImVec2(120, 0))) {
                m_show_progress = false;
            }
        } else {
            if (ImGui::Button("取消", ImVec2(120, 0))) {
                cancel_merge();
            }
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// 功能方法实现
// ============================================================================

void FFmpegView::add_files() {
    LOG_INFO("FFmpegView: Add files requested");

    // 使用文件对话框选择多个 TS 文件
    using namespace Core::Utils;

    FileDialogOptions options;
    options.title = "选择 TS 文件";
    options.filters.push_back(FileFilter("视频文件", "*.ts;*.mts;*.m2ts"));
    options.filters.push_back(FileFilter("所有文件", "*.*"));
    options.allow_multiple = true;  // 允许多选

    const FileDialogResult result = FileDialog::open_file(options);

    if (result.success && result.has_paths()) {
        LOG_INFO("FFmpegView: User selected {} files", result.paths.size());

        // 直接将所有选中的文件添加到主列表
        for (const auto& path : result.paths) {
            TSFile file;
            file.path = path.string();
            file.name = path.filename().string();

            // 获取文件大小
            try {
                file.size = std::filesystem::file_size(path);
            } catch (...) {
                file.size = 0;
            }

            // 获取时长（使用 FFmpeg）
            file.duration = 0.0;
            file.valid = true;

            m_files.push_back(file);
            LOG_INFO("FFmpegView: Added file: {} ({})",
                file.name, format_file_size(file.size));
        }

        LOG_INFO("FFmpegView: Total files in list: {}", m_files.size());
    } else {
        LOG_INFO("FFmpegView: File selection cancelled");
    }
}

void FFmpegView::add_folder() {
    LOG_INFO("FFmpegView: Add folder requested");

    // 使用文件夹对话框
    using namespace Core::Utils;

    std::filesystem::path folder_path = select_single_folder("选择包含 TS 文件的文件夹");

    if (!folder_path.empty()) {
        m_current_folder = folder_path.string();

        // 扫描文件夹中的 TS 文件
        scan_folder(m_current_folder);

        if (!m_detected_files.empty()) {
            // 直接添加所有文件到主列表（简化用户体验）
            LOG_INFO("FFmpegView: Adding {} files to main list", m_detected_files.size());
            for (auto& file : m_detected_files) {
                m_files.push_back(std::move(file));
            }
            m_detected_files.clear();
            LOG_INFO("FFmpegView: Total files in main list: {}", m_files.size());
        } else {
            LOG_WARN("FFmpegView: No .ts files found in folder: {}", m_current_folder);
        }
    } else {
        LOG_INFO("FFmpegView: Folder selection cancelled");
    }
}

void FFmpegView::confirm_file_selection() {
    LOG_INFO("FFmpegView: User confirmed %zu files", m_detected_files.size());

    // 将检测到的文件添加到主列表
    for (auto& file : m_detected_files) {
        m_files.push_back(std::move(file));
    }

    m_detected_files.clear();
    m_show_file_selector = false;
}

void FFmpegView::scan_folder(const std::string& folder_path) {
    LOG_INFO("FFmpegView: Scanning folder: {}", folder_path);

    m_detected_files.clear();

    try {
        std::filesystem::path folder(folder_path);

        // 检查文件夹是否存在
        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
            LOG_ERROR("FFmpegView: Invalid folder path: {}", folder_path);
            return;
        }

        // 遍历文件夹查找 .ts 文件
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                if (is_ts_file(path.filename().string())) {
                    TSFile file;
                    file.path = path.string();
                    file.name = path.filename().string();

                    // 获取文件大小
                    try {
                        file.size = std::filesystem::file_size(path);
                    } catch (...) {
                        file.size = 0;
                    }

                    // 暂时设置时长为 0（后续可以用 FFmpeg 获取）
                    file.duration = 0.0;
                    file.valid = true;

                    m_detected_files.push_back(file);
                    LOG_DEBUG("FFmpegView: Found TS file: {}", file.name);
                }
            }
        }

        // 按文件名排序
        std::sort(m_detected_files.begin(), m_detected_files.end(),
                  [](const TSFile& a, const TSFile& b) {
                      return a.name < b.name;
                  });

        LOG_INFO("FFmpegView: Scan complete, found {} TS files", m_detected_files.size());

    } catch (const std::exception& e) {
        LOG_ERROR("FFmpegView: Failed to scan folder: {}", e.what());
    }
}

bool FFmpegView::is_ts_file(const std::string& filename) {
    // 检查文件扩展名是否为 .ts
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == "ts";
}

void FFmpegView::remove_file(size_t index) {
    if (index < m_files.size()) {
        m_files.erase(m_files.begin() + index);
        if (m_selected_file >= m_files.size()) {
            m_selected_file = m_files.empty() ? SIZE_MAX : m_files.size() - 1;
        }
    }
}

void FFmpegView::clear_files() {
    m_files.clear();
    m_selected_file = SIZE_MAX;
}

void FFmpegView::move_file_up(size_t index) {
    if (index > 0 && index < m_files.size()) {
        std::swap(m_files[index], m_files[index - 1]);
        if (m_selected_file == index) {
            m_selected_file = index - 1;
        } else if (m_selected_file == index - 1) {
            m_selected_file = index;
        }
    }
}

void FFmpegView::move_file_down(size_t index) {
    if (index < m_files.size() - 1) {
        std::swap(m_files[index], m_files[index + 1]);
        if (m_selected_file == index) {
            m_selected_file = index + 1;
        } else if (m_selected_file == index + 1) {
            m_selected_file = index;
        }
    }
}

// ============================================================================
// FFmpeg 合并实现
// ============================================================================

void FFmpegView::start_merge() {
    if (!validate_files()) {
        return;
    }

    LOG_INFO("FFmpegView: Starting merge of {} files", m_files.size());

    // 让用户选择输出文件路径
    using namespace Core::Utils;

    FileDialogOptions save_options;
    save_options.title = "Save Video Files";
    save_options.filters.push_back(FileFilter("all file", "*.*"));

    FileDialogResult save_result = FileDialog::save_file(save_options);

    if (!save_result.success || !save_result.has_paths()) {
        LOG_INFO("FFmpegView: Output file selection cancelled");
        return;
    }

    m_output_path = save_result.paths[0].string();

    // 确保文件扩展名是 .mp4
    if (!m_output_path.ends_with(".mp4")) {
        m_output_path += ".mp4";
    }

    LOG_INFO("FFmpegView: Output path: {}", m_output_path);

    // 使用 TaskManager 启动合并任务
    using namespace Core::Tasks;

    m_merge_progress.active = true;
    m_merge_progress.progress = 0.0;
    m_merge_progress.current_file = 0;
    m_merge_progress.total_files = static_cast<int>(m_files.size());
    m_merge_progress.current_action = "初始化...";

    // 启动任务
    LOG_INFO("FFmpegView: Launching merge task...");

    m_merge_task = TaskManager::instance().launch(
        "合并 TS 文件",
        [this](const std::atomic<bool>& should_cancel) {
            LOG_INFO("FFmpegView: >>> Merge task lambda started, should_cancel address: {}", (void*)&should_cancel);

            bool success = merge_ts_files(m_output_path, should_cancel);

            LOG_INFO("FFmpegView: >>> Merge task lambda finished, success: {}", success);

            if (!success) {
                // 如果失败，抛出异常让 TaskManager 捕获并发送 TaskFailedEvent
                std::string error_msg = m_merge_progress.error_message.empty()
                    ? "合并失败，未知错误"
                    : m_merge_progress.error_message;

                LOG_ERROR("FFmpegView: >>> Throwing exception: {}", error_msg);
                throw std::runtime_error(error_msg);
            }

            LOG_INFO("FFmpegView: >>> Task completed successfully, about to return from lambda");
        },
        TaskType::Background
    );

    LOG_INFO("FFmpegView: Merge task launched, task pointer: {}", (void*)m_merge_task.get());
}

void FFmpegView::cancel_merge() {
    // 安全检查：确保任务指针有效
    if (m_merge_task && m_merge_task.use_count() > 0) {
        try {
            if (m_merge_task->isRunning()) {
                LOG_INFO("FFmpegView: Cancelling merge task");
                m_merge_task->cancel();
            }
        } catch (...) {
            LOG_WARN("FFmpegView: Failed to cancel task (may be destroyed)");
        }
    }
    m_merge_progress.active = false;
    m_merge_progress.current_action = "Cancelled";

    // 释放任务指针，避免事件处理器中继续访问
    m_merge_task.reset();
}

bool FFmpegView::validate_files() {
    if (m_files.empty()) {
        m_merge_progress.error_message = "没有文件可合并";
        return false;
    }

    if (!std::all_of(m_files.begin(), m_files.end(),
                      [](const TSFile& f) { return f.valid; })) {
        m_merge_progress.error_message = "部分文件无效";
        return false;
    }

    return true;
}

std::string FFmpegView::create_concat_list() {
    // 创建 concat 列表文件
    auto temp_dir = std::filesystem::temp_directory_path();
    auto temp_file = temp_dir / "dearts_concat_list.txt";

    // 如果文件已存在，先删除
    if (std::filesystem::exists(temp_file)) {
        std::error_code ec;
        std::filesystem::remove(temp_file, ec);
    }

    std::ofstream list_file(temp_file);
    if (!list_file.is_open()) {
        LOG_ERROR("FFmpegView: Failed to create concat list file at: {}", temp_file.string());
        return "";
    }

    for (const auto& file : m_files) {
        // 获取要写入的路径
        std::string write_path = file.path;

#ifdef _WIN32
        // 在 Windows 上尝试使用短路径名来避免中文和特殊字符问题
        std::string short_path = FFmpegView::get_short_path_name(file.path);
        if (short_path != file.path) {
            // 成功获取短路径名，使用它
            write_path = short_path;
            LOG_DEBUG("FFmpegView: Using short path: {} -> {}", file.path, short_path);
        } else {
            LOG_WARN("FFmpegView: Could not get short path for: {}", file.path);
        }
#endif

        // FFmpeg concat 格式需要对路径进行转义
        // Windows 路径中的反斜杠转换为正斜杠
        for (auto& ch : write_path) {
            if (ch == '\\') {
                ch = '/';
            }
        }

        // 对于包含特殊字符的路径，使用反斜杠转义
        // FFmpeg concat 协议的转义规则：单引号和反斜杠需要转义
        std::string final_path;
        for (char ch : write_path) {
            if (ch == '\'') {
                final_path += "\\'";
            } else if (ch == '\\') {
                final_path += "\\\\";
            } else {
                final_path += ch;
            }
        }

        list_file << "file '" << final_path << "'\n";
        LOG_DEBUG("FFmpegView: Added to concat list: {}", final_path);
    }

    list_file.close();

    // 调试：输出 concat 列表内容
    std::ifstream check_list(temp_file);
    std::string line;
    int line_num = 0;
    LOG_INFO("FFmpegView: Concat list content:");
    while (std::getline(check_list, line) && line_num < 5) {
        LOG_INFO("  {}", line);
        line_num++;
    }
    check_list.close();

    std::string list_path = temp_file.string();
    LOG_INFO("FFmpegView: Created concat list: {}", list_path);
    return list_path;
}

bool FFmpegView::merge_ts_files(const std::string& output_path, const std::atomic<bool>& should_cancel) {
    // 创建 concat 列表
    std::string concat_list = create_concat_list();
    if (concat_list.empty()) {
        m_merge_progress.error_message = "创建合并列表失败";
        return false;
    }

    LOG_INFO("FFmpegView: Starting merge to {}", output_path);

    // 解析编码器类型
    EncoderType actual_encoder = resolve_encoder_type(m_selected_encoder);
    const char* video_encoder = get_encoder_name(actual_encoder);

    LOG_INFO("FFmpegView: Using encoder: {}", video_encoder ? video_encoder : "copy");

    // 构建 FFmpeg 命令行
    // 使用 ffmpeg 命令行工具来避免 C API 的路径安全问题
    std::string ffmpeg_cmd;

    // 对路径中的特殊字符进行转义
    auto escape_path = [](const std::string& path) -> std::string {
#ifdef _WIN32
        // Windows: 使用双引号包围路径
        std::string result = "\"";
        for (char ch : path) {
            if (ch == '\\') {
                result += "\\\\";
            } else if (ch == '"') {
                result += "\\\"";
            } else {
                result += ch;
            }
        }
        result += "\"";
        return result;
#else
        // Linux/Unix: 使用单引号包围路径
        return "'" + path + "'";
#endif
    };

    // 构建 FFmpeg 命令
    ffmpeg_cmd = "ffmpeg -y -f concat -safe 0 -i " +
                 escape_path(concat_list);

    // 根据编码器类型添加视频编码参数
    if (video_encoder) {
        // 使用指定的编码器（重新编码）
        ffmpeg_cmd += " -c:v " + std::string(video_encoder);

        // 添加编码器特定参数
        switch (actual_encoder) {
            case EncoderType::NVIDIA_NVENC:
                // NVIDIA NVENC 参数
                ffmpeg_cmd += " -preset p4 -rc vbr -b:v 5M -maxrate 10M -bufsize 20M";
                break;
            case EncoderType::AMD_VCE:
                // AMD AMF 参数
                ffmpeg_cmd += " -quality speed -bitrate 5000k -maxrate 10000k -bufsize 20000k";
                break;
            case EncoderType::Intel_QSV:
                // Intel QSV 参数
                ffmpeg_cmd += " -preset faster -global_quality 25 -look_ahead 1";
                break;
            case EncoderType::CPU:
                // libx264 参数
                ffmpeg_cmd += " -preset medium -crf 23";
                break;
            default:
                break;
        }

        // 音频直接复制
        ffmpeg_cmd += " -c:a copy";
    } else {
        // 没有指定编码器，直接复制流（不重新编码）
        ffmpeg_cmd += " -c copy";
    }

    ffmpeg_cmd += " " + escape_path(output_path);

    LOG_INFO("FFmpegView: Executing: {}", ffmpeg_cmd);

    // 执行 FFmpeg 命令
    m_merge_progress.current_action = "正在合并文件...";
    m_merge_progress.progress = 0.2;

#ifdef _WIN32
    FILE* pipe = _popen(ffmpeg_cmd.c_str(), "r");
#else
    FILE* pipe = popen(ffmpeg_cmd.c_str(), "r");
#endif

    if (!pipe) {
        m_merge_progress.error_message = "无法启动 FFmpeg 进程";
        LOG_ERROR("FFmpegView: {}", m_merge_progress.error_message);
        return false;
    }

    // 读取 FFmpeg 输出以获取进度
    char buffer[512];
    int frame_count = 0;
    int line_count = 0;
    m_merge_progress.total_files = static_cast<int>(m_files.size());

    LOG_INFO("FFmpegView: Starting to read FFmpeg output...");

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        line_count++;

        // 每 100 行记录一次日志，确认还在运行
        if (line_count % 100 == 0) {
            LOG_INFO("FFmpegView: FFmpeg is running, processed {} lines of output", line_count);
        }

        // 解析 FFmpeg 输出获取进度
        // FFmpeg 输出格式: frame= 123 fps= 50 q=28.0 size= 1234kB time=00:00:05.00 bitrate= 1234.5kbits/s speed=1.23x

        // 检查是否被取消
        if (should_cancel.load()) {
            LOG_INFO("FFmpegView: Merge cancelled by user");
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif

            // 删除输出文件
            std::error_code ec;
            std::filesystem::remove(output_path, ec);

            m_merge_progress.error_message = "操作已取消";
            return false;
        }

        // 解析进度
        if (strstr(buffer, "frame=") != nullptr) {
            frame_count++;
            m_merge_progress.current_file = frame_count;

            // 估算进度（基于时间）
            if (strstr(buffer, "time=") != nullptr) {
                char time_str[32];
                if (sscanf(strstr(buffer, "time="), "time=%31s", time_str) == 1) {
                    // 解析时间字符串 HH:MM:SS.ms
                    int hours = 0, minutes = 0, seconds = 0;
                    if (sscanf(time_str, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
                        int total_seconds = hours * 3600 + minutes * 60 + seconds;
                        // 假设总时长约 9200 秒（2小时33分），根据实际视频时长调整
                        double progress = std::min(0.95, static_cast<double>(total_seconds) / 9200.0);
                        m_merge_progress.progress.store(progress);

                        // 每 1000 帧记录一次进度
                        if (frame_count % 1000 == 0) {
                            LOG_INFO("FFmpegView: Progress: {:.1f}%, Time: {}", progress * 100, time_str);
                        }
                    }
                }
            }
        }

        // 检查错误
        if (strstr(buffer, "Error") != nullptr || strstr(buffer, "error") != nullptr) {
            LOG_WARN("FFmpegView: FFmpeg output: {}", buffer);
        }

        // 记录 FFmpeg 的 map 输出（流信息）
        if (strstr(buffer, "Stream #0") != nullptr) {
            LOG_INFO("FFmpegView: {}", buffer);
        }
    }

    LOG_INFO("FFmpegView: Finished reading FFmpeg output, total lines: {}, frames: {}", line_count, frame_count);

    // 等待 FFmpeg 完成
    int exit_code = 0;
#ifdef _WIN32
    exit_code = _pclose(pipe);
#else
    exit_code = pclose(pipe);
#endif

    if (exit_code != 0) {
        m_merge_progress.error_message = std::format("FFmpeg 执行失败，退出码: {}", exit_code);
        LOG_ERROR("FFmpegView: {}", m_merge_progress.error_message);
        return false;
    }

    // 检查输出文件是否存在
    if (!std::filesystem::exists(output_path)) {
        m_merge_progress.error_message = "输出文件未创建";
        LOG_ERROR("FFmpegView: {}", m_merge_progress.error_message);
        return false;
    }

    m_merge_progress.progress = 1.0;
    m_merge_progress.current_action = "合并完成！";

    LOG_INFO("FFmpegView: ============================================)");
    LOG_INFO("FFmpegView: Merge completed successfully!");
    LOG_INFO("FFmpegView: Output file: {}", output_path);
    LOG_INFO("FFmpegView: Returning true to trigger TaskCompletedEvent");
    LOG_INFO("FFmpegView: ============================================");
    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string FFmpegView::get_short_path_name(const std::string& long_path) {
#ifdef _WIN32
    // 首先获取需要的缓冲区大小
    DWORD size = GetShortPathNameA(long_path.c_str(), nullptr, 0);
    if (size == 0) {
        // 如果无法获取短路径名，返回原始路径
        return long_path;
    }

    // 分配缓冲区并获取短路径名
    std::vector<char> buffer(size);
    DWORD result = GetShortPathNameA(long_path.c_str(), buffer.data(), size);
    if (result == 0) {
        // 失败，返回原始路径
        return long_path;
    }

    return std::string(buffer.data());
#else
    // 非 Windows 平台，直接返回原始路径
    return long_path;
#endif
}

std::string FFmpegView::format_file_size(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size_d = static_cast<double>(size);

    while (size_d >= 1024.0 && unit < 3) {
        size_d /= 1024.0;
        unit++;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.1f %s", size_d, units[unit]);
    return std::string(buffer);
}

std::string FFmpegView::format_duration(double seconds) {
    int hours = static_cast<int>(seconds) / 3600;
    int minutes = (static_cast<int>(seconds) % 3600) / 60;
    int secs = static_cast<int>(seconds) % 60;

    char buffer[64];
    if (hours > 0) {
        snprintf(buffer, sizeof(buffer), "%d:%02d:%02d", hours, minutes, secs);
    } else {
        snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, secs);
    }
    return std::string(buffer);
}

// ============================================================================
// 文件选择对话框
// ============================================================================

void FFmpegView::draw_file_selector() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("选择TS文件", &m_show_file_selector,
                                 ImGuiWindowFlags_NoResize)) {
        // 文件夹信息
        ImGui::Text("文件夹: %s", m_current_folder.c_str());
        ImGui::Text("找到 %zu 个 .ts 文件", m_detected_files.size());

        ImGui::Separator();
        ImGui::Spacing();

        // 全选/取消按钮
        if (ImGui::Button("全选")) {
            for (auto& file : m_detected_files) {
                file.valid = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("取消全选")) {
            for (auto& file : m_detected_files) {
                file.valid = false;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("按模式选择")) {
            // 按文件名模式选择（例如连续编号）
            bool in_sequence = true;
            size_t first_num = 0;

            for (size_t i = 0; i < m_detected_files.size(); ++i) {
                // 提取文件名中的数字
                size_t num_pos = m_detected_files[i].name.find_first_of("0123456789");
                if (num_pos != std::string::npos) {
                    size_t num = 0;
                    sscanf(m_detected_files[i].name.c_str() + num_pos, "%zu", &num);

                    if (i == 0) {
                        first_num = num;
                    } else if (num != first_num + i) {
                        in_sequence = false;
                        break;
                    }
                }
            }

            // 如果是连续编号，全选
            if (in_sequence && !m_detected_files.empty()) {
                for (auto& file : m_detected_files) {
                    file.valid = true;
                }
                LOG_INFO("FFmpegView: Selected sequential files");
            }
        }

        ImGui::Spacing();

        // 文件列表（带复选框）
        ImVec2 list_size = ImVec2(-1, 320);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 0.5f));
        if (ImGui::BeginChild("FileSelectorList", list_size, true)) {
            for (size_t i = 0; i < m_detected_files.size(); ++i) {
                auto& file = m_detected_files[i];

                // 复选框
                char label[256];
                snprintf(label, sizeof(label), "##select_%zu", i);

                bool selected = file.valid;
                if (ImGui::Checkbox(label, &selected)) {
                    file.valid = selected;
                }

                ImGui::SameLine();

                // 文件信息
                ImGui::BeginGroup();
                ImGui::Text("%s", file.name.c_str());

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
                ImGui::Text("%s | %s",
                    format_file_size(file.size).c_str(),
                    format_duration(file.duration).c_str());
                ImGui::PopStyleColor();

                ImGui::EndGroup();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // 统计信息
        size_t selected_count = std::count_if(m_detected_files.begin(), m_detected_files.end(),
                                               [](const TSFile& f) { return f.valid; });
        size_t total_size = 0;
        double total_duration = 0.0;

        for (const auto& file : m_detected_files) {
            if (file.valid) {
                total_size += file.size;
                total_duration += file.duration;
            }
        }

        ImGui::Text("已选择: %zu / %zu", selected_count, m_detected_files.size());
        ImGui::SameLine();
        ImGui::Text("总计: %s | %s",
            format_file_size(total_size).c_str(),
            format_duration(total_duration).c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // 确认和取消按钮
        float button_width = 120;

        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button_width * 2 - 10);

        if (ImGui::Button("取消", ImVec2(button_width, 0))) {
            m_detected_files.clear();
            m_show_file_selector = false;
        }

        ImGui::SameLine();

        bool has_selection = selected_count > 0;
        if (!has_selection) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("确认", ImVec2(button_width, 0))) {
            confirm_file_selection();
        }

        if (!has_selection) {
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();
    }
}

void FFmpegView::load_file_info(TSFile& file) {
    // TODO: 使用 FFmpeg 获取文件信息
    // avformat_open_input() 等
}

// ============================================================================
// 硬件检测
// ============================================================================

HardwareInfo FFmpegView::detect_hardware() {
    HardwareInfo info;

    LOG_INFO("FFmpegView: Detecting hardware encoders...");

    // 检测 NVIDIA NVENC (h264_nvenc)
    const AVCodec* nvenc_codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (nvenc_codec) {
        info.has_nvidia_gpu = true;
        info.nvidia_gpu_name = "NVIDIA GPU (NVENC)";
        info.recommended_encoder = EncoderType::NVIDIA_NVENC;
        LOG_INFO("FFmpegView: Found NVIDIA NVENC encoder");
    }

    // 检测 AMD VCE (h264_amf)
    const AVCodec* amf_codec = avcodec_find_encoder_by_name("h264_amf");
    if (amf_codec) {
        info.has_amd_gpu = true;
        info.amd_gpu_name = "AMD GPU (AMF)";
        if (!info.has_nvidia_gpu) {
            info.recommended_encoder = EncoderType::AMD_VCE;
        }
        LOG_INFO("FFmpegView: Found AMD AMF encoder");
    }

    // 检测 Intel Quick Sync (h264_qsv)
    const AVCodec* qsv_codec = avcodec_find_encoder_by_name("h264_qsv");
    if (qsv_codec) {
        info.has_intel_gpu = true;
        info.intel_gpu_name = "Intel GPU (Quick Sync)";
        if (!info.has_nvidia_gpu && !info.has_amd_gpu) {
            info.recommended_encoder = EncoderType::Intel_QSV;
        }
        LOG_INFO("FFmpegView: Found Intel QSV encoder");
    }

    // 如果没有找到硬件编码器，使用 CPU
    if (!info.has_nvidia_gpu && !info.has_amd_gpu && !info.has_intel_gpu) {
        info.recommended_encoder = EncoderType::CPU;
        LOG_INFO("FFmpegView: No hardware encoders found, will use CPU encoding");
    }

    return info;
}

bool FFmpegView::check_encoder_available(EncoderType type) {
    const char* encoder_name = get_encoder_name(type);
    if (!encoder_name) {
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder_by_name(encoder_name);
    return codec != nullptr;
}

const char* FFmpegView::get_encoder_name(EncoderType type) {
    switch (type) {
        case EncoderType::NVIDIA_NVENC:
            return "h264_nvenc";
        case EncoderType::AMD_VCE:
            return "h264_amf";
        case EncoderType::Intel_QSV:
            return "h264_qsv";
        case EncoderType::CPU:
            return "libx264";
        case EncoderType::Auto:
            return nullptr;  // Auto 需要特殊处理
        default:
            return nullptr;
    }
}

EncoderType FFmpegView::resolve_encoder_type(EncoderType type) {
    if (type != EncoderType::Auto) {
        return type;
    }

    // Auto 模式：根据硬件检测结果选择最佳编码器
    if (m_hardware_info.has_nvidia_gpu) {
        return EncoderType::NVIDIA_NVENC;
    } else if (m_hardware_info.has_amd_gpu) {
        return EncoderType::AMD_VCE;
    } else if (m_hardware_info.has_intel_gpu) {
        return EncoderType::Intel_QSV;
    } else {
        return EncoderType::CPU;
    }
}

// ============================================================================
// 文件选择方法
// ============================================================================

void FFmpegView::select_all_files() {
    m_selected_files_set.clear();
    for (size_t i = 0; i < m_files.size(); ++i) {
        m_selected_files_set.insert(i);
    }
    LOG_INFO("FFmpegView: Selected all {} files", m_files.size());
}

void FFmpegView::deselect_all_files() {
    m_selected_files_set.clear();
    LOG_DEBUG("FFmpegView: Deselected all files");
}

void FFmpegView::remove_selected_files() {
    if (m_selected_files_set.empty()) {
        return;
    }

    // 从后往前删除，避免索引问题
    std::vector<size_t> selected_indices(m_selected_files_set.begin(), m_selected_files_set.end());
    std::sort(selected_indices.begin(), selected_indices.end(), std::greater<size_t>());

    for (size_t index : selected_indices) {
        if (index < m_files.size()) {
            LOG_INFO("FFmpegView: Removing file at index {}: {}", index, m_files[index].name);
            m_files.erase(m_files.begin() + index);
        }
    }

    m_selected_files_set.clear();
    m_selected_file = SIZE_MAX;

    LOG_INFO("FFmpegView: Removed {} files, {} remaining", selected_indices.size(), m_files.size());
}

bool FFmpegView::is_file_selected(size_t index) const {
    return m_selected_files_set.find(index) != m_selected_files_set.end();
}

void FFmpegView::toggle_file_selection(size_t index) {
    if (is_file_selected(index)) {
        m_selected_files_set.erase(index);
    } else {
        m_selected_files_set.insert(index);
    }
}

} // namespace DearTs::Plugins::FFmpeg

#endif // DEARTS_FFMPEG_SUPPORT
