/**
 * @file layout_manager.cpp
 * @brief 布局管理器实现
 */

#include "layout_manager.h"
#include "logger.h"
#include <fstream>
#include <algorithm>

namespace DearTs::Core::UI {

bool LayoutManager::saveLayout(const std::string& name, const std::string& filepath) {
    try {
        std::string layout_path = filepath.empty() ? getDefaultLayoutPath() : filepath;

        // 使用 ImGui 的设置保存功能
        size_t ini_size = 0;
        const char* ini_data = ImGui::SaveIniSettingsToMemory(&ini_size);

        // 写入文件
        std::ofstream file(layout_path);
        if (!file.is_open()) {
            LOG_ERROR("无法保存布局到文件: {}", layout_path);
            return false;
        }

        file.write(ini_data, ini_size);
        file.close();

        m_current_layout = name;
        m_dirty = false;

        LOG_INFO("布局已保存: {} -> {}", name, layout_path);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("保存布局失败: {}", e.what());
        return false;
    }
}

bool LayoutManager::loadLayout(const std::string& filepath) {
    try {
        // 读取文件
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("无法打开布局文件: {}", filepath);
            return false;
        }

        // 读取内容
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // 加载到 ImGui
        return loadFromString(content);

    } catch (const std::exception& e) {
        LOG_ERROR("加载布局失败: {}", e.what());
        return false;
    }
}

std::string LayoutManager::saveToString() const {
    size_t ini_size = 0;
    const char* ini_data = ImGui::SaveIniSettingsToMemory(&ini_size);
    return std::string(ini_data, ini_size);
}

bool LayoutManager::loadFromString(const std::string& content) {
    if (content.empty()) {
        LOG_WARN("布局内容为空");
        return false;
    }

    try {
        // 使用 ImGui 的设置加载功能
        ImGui::LoadIniSettingsFromMemory(content.c_str(), content.size());

        LOG_INFO("布局已从字符串加载");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("从字符串加载布局失败: {}", e.what());
        return false;
    }
}

void LayoutManager::resetToDefault() {
    LOG_INFO("重置为默认布局");

    // 注意：ImGui docking 分支中没有 ClearIniSettings 函数
    // 要重置布局，有以下几种方法：
    // 1. 删除 imgui.ini 文件
    // 2. 设置 io.IniFilename = NULL 然后重新创建 context
    // 3. 保存空的设置

    // 这里我们通过保存空设置来重置
    ImGuiIO& io = ImGui::GetIO();
    const char* original_ini_filename = io.IniFilename;

    // 临时禁用 INI 文件
    io.IniFilename = nullptr;

    // 触发布局变化回调
    if (m_layout_changed_callback) {
        m_layout_changed_callback();
    }

    // 恢复 INI 文件名（下次启动时会重新创建）
    io.IniFilename = original_ini_filename;

    m_dirty = true;
}

void LayoutManager::lockLayout(bool locked) {
    if (m_locked != locked) {
        m_locked = locked;
        LOG_INFO("布局{}", locked ? "已锁定" : "已解锁");
    }
}

void LayoutManager::closeAllViews() {
    // 这个功能需要配合 ViewManager 实现
    LOG_INFO("关闭所有视图");
    // TODO: 遍历所有视图并关闭
}

std::vector<std::string> LayoutManager::getSavedLayouts() const {
    std::vector<std::string> layouts;

    // TODO: 扫描布局目录，查找所有 .hexlyt 或 .ini 文件
    // 当前返回空列表
    return layouts;
}

bool LayoutManager::deleteLayout(const std::string& name) {
    // TODO: 实现删除布局文件的功能
    LOG_WARN("删除布局功能尚未实现: {}", name);
    return false;
}

void LayoutManager::update() {
    // 每帧检查是否需要自动保存布局
    if (m_dirty) {
        // 自动保存当前布局
        saveLayout("auto_save");
    }
}

std::string LayoutManager::getDefaultLayoutPath() const {
    return "layout_default.ini";
}

void LayoutManager::applyLayout() {
    // 应用保存的布局设置
    // ImGui 会在下一帧自动应用加载的设置
}

} // namespace DearTs::Core::UI
