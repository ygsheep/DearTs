/**
 * @file project_manager.cpp
 * @brief 项目管理器实现
 */

#include "project_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace DearTs::Core::ContentRegistry {

// ================ Project Implementation ================

Project::Project(const ProjectMetadata& metadata)
    : m_metadata(metadata)
    , m_state(ProjectState::Opened)
{
    // 设置创建日期
    if (m_metadata.creation_date.empty()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
        m_metadata.creation_date = ss.str();
    }

    // 设置最后修改日期
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    m_metadata.last_modified = ss.str();
}

void Project::addFile(const ProjectFile& file) {
    m_files.push_back(file);
    markModified();
}

void Project::removeFile(const std::string& path) {
    auto it = std::remove_if(m_files.begin(), m_files.end(),
        [&path](const ProjectFile& f) {
            return f.path == path;
        });

    if (it != m_files.end()) {
        m_files.erase(it, m_files.end());
        markModified();
    }
}

// ================ ProjectManager Implementation ================

std::shared_ptr<Project> ProjectManager::createProject(const ProjectMetadata& metadata) {
    auto project = std::make_shared<Project>(metadata);
    m_current_project = project;

    LOG_INFO("Created project: {}", metadata.name);
    return project;
}

std::shared_ptr<Project> ProjectManager::openProject(const std::string& filepath) {
    // 读取项目文件
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open project file: {}", filepath);
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // 解析 JSON
    auto project = projectFromJSON(buffer.str());
    if (!project) {
        LOG_ERROR("Failed to parse project file: {}", filepath);
        return nullptr;
    }

    project->setState(ProjectState::Opened);
    m_current_project = project;

    // 添加到最近项目列表
    addRecentProject(filepath);

    LOG_INFO("Opened project: {} from {}", project->getMetadata().name, filepath);
    return project;
}

bool ProjectManager::saveProject(std::shared_ptr<Project> project, const std::string& filepath) {
    if (!project) {
        LOG_ERROR("Cannot save null project");
        return false;
    }

    std::string save_path = filepath.empty() ? project->getMetadata().filepath : filepath;

    if (save_path.empty()) {
        LOG_ERROR("No filepath specified for saving project");
        return false;
    }

    // 更新最后修改时间
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    project->getMetadata().last_modified = ss.str();

    // 生成 JSON
    std::string json_str = projectToJSON(project);

    // 写入文件
    std::ofstream file(save_path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create project file: {}", save_path);
        return false;
    }

    file << json_str;
    file.close();

    // 更新项目状态
    project->setState(ProjectState::Opened);

    // 保存文件路径到元数据
    auto& metadata = project->getMetadata();
    metadata.filepath = save_path;

    // 添加到最近项目列表
    addRecentProject(save_path);

    LOG_INFO("Saved project: {} to {}", project->getMetadata().name, save_path);
    return true;
}

void ProjectManager::closeProject(std::shared_ptr<Project> project) {
    if (!project) {
        return;
    }

    LOG_INFO("Closing project: {}", project->getMetadata().name);

    if (m_current_project == project) {
        m_current_project = nullptr;
    }
}

void ProjectManager::addRecentProject(const std::string& filepath) {
    // 移除重复项
    auto it = std::remove(m_recent_projects.begin(), m_recent_projects.end(), filepath);
    m_recent_projects.erase(it, m_recent_projects.end());

    // 添加到开头
    m_recent_projects.insert(m_recent_projects.begin(), filepath);

    // 限制数量（最多 10 个）
    if (m_recent_projects.size() > 10) {
        m_recent_projects.resize(10);
    }
}

void ProjectManager::clearRecentProjects() {
    m_recent_projects.clear();
    LOG_INFO("Cleared recent projects list");
}

bool ProjectManager::saveRecentProjects(const std::string& config_path) {
    std::string path = config_path.empty() ? "recent_projects.json" : config_path;

    // 简单的 JSON 数组
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to save recent projects to: {}", path);
        return false;
    }

    file << "[\n";
    for (size_t i = 0; i < m_recent_projects.size(); ++i) {
        file << "  \"" << m_recent_projects[i] << "\"";
        if (i < m_recent_projects.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    file << "]\n";

    file.close();
    LOG_INFO("Saved {} recent projects to {}", m_recent_projects.size(), path);
    return true;
}

bool ProjectManager::loadRecentProjects(const std::string& config_path) {
    std::string path = config_path.empty() ? "recent_projects.json" : config_path;

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("No recent projects file found: {}", path);
        return false;
    }

    m_recent_projects.clear();

    std::string line;
    while (std::getline(file, line)) {
        // 简单的解析：提取引号中的路径
        size_t start = line.find('"');
        if (start != std::string::npos) {
            size_t end = line.rfind('"');
            if (end != std::string::npos && end > start) {
                std::string filepath = line.substr(start + 1, end - start - 1);
                if (!filepath.empty()) {
                    m_recent_projects.push_back(filepath);
                }
            }
        }
    }

    file.close();
    LOG_INFO("Loaded {} recent projects from {}", m_recent_projects.size(), path);
    return true;
}

bool ProjectManager::needsSave() const {
    return m_current_project && m_current_project->hasUnsavedChanges();
}

void ProjectManager::autoSave() {
    if (!m_current_project || !m_current_project->isModified()) {
        return;
    }

    const auto& metadata = m_current_project->getMetadata();
    if (!metadata.auto_save || metadata.filepath.empty()) {
        return;
    }

    LOG_INFO("Auto-saving project: {}", metadata.name);
    saveProject(m_current_project);

    if (m_auto_save_callback) {
        m_auto_save_callback(m_current_project);
    }
}

std::string ProjectManager::projectToJSON(std::shared_ptr<Project> project) {
    if (!project) {
        return "{}";
    }

    const auto& meta = project->getMetadata();
    const auto& files = project->getFiles();

    std::stringstream json;

    json << "{\n";
    json << "  \"version\": \"1.0\",\n";
    json << "  \"metadata\": {\n";
    json << "    \"name\": \"" << meta.name << "\",\n";
    json << "    \"description\": \"" << meta.description << "\",\n";
    json << "    \"author\": \"" << meta.author << "\",\n";
    json << "    \"creation_date\": \"" << meta.creation_date << "\",\n";
    json << "    \"last_modified\": \"" << meta.last_modified << "\",\n";
    json << "    \"version\": \"" << meta.version << "\",\n";
    json << "    \"auto_save\": " << (meta.auto_save ? "true" : "false") << ",\n";
    json << "    \"auto_save_interval\": " << meta.auto_save_interval << "\n";
    json << "  },\n";

    // 标签
    json << "  \"tags\": [";
    for (size_t i = 0; i < meta.tags.size(); ++i) {
        json << "\"" << meta.tags[i] << "\"";
        if (i < meta.tags.size() - 1) {
            json << ", ";
        }
    }
    json << "],\n";

    // 文件
    json << "  \"files\": [\n";
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& file = files[i];
        json << "    {\n";
        json << "      \"path\": \"" << file.path << "\",\n";
        json << "      \"type\": \"" << file.type << "\",\n";
        json << "      \"size\": " << file.size << ",\n";
        json << "      \"readonly\": " << (file.readonly ? "true" : "false") << "\n";
        json << "    }";
        if (i < files.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ]\n";

    json << "}\n";

    return json.str();
}

std::shared_ptr<Project> ProjectManager::projectFromJSON(const std::string& json_str) {
    // 简单的 JSON 解析（生产环境应使用专门的 JSON 库）
    ProjectMetadata metadata;

    // 提取元数据
    size_t pos = 0;

    // 提取 name
    pos = json_str.find("\"name\": \"", pos);
    if (pos != std::string::npos) {
        pos += 9;
        size_t end = json_str.find("\"", pos);
        metadata.name = json_str.substr(pos, end - pos);
    }

    // 提取 description
    pos = json_str.find("\"description\": \"", pos);
    if (pos != std::string::npos) {
        pos += 16;
        size_t end = json_str.find("\"", pos);
        metadata.description = json_str.substr(pos, end - pos);
    }

    // 提取 author
    pos = json_str.find("\"author\": \"", pos);
    if (pos != std::string::npos) {
        pos += 11;
        size_t end = json_str.find("\"", pos);
        metadata.author = json_str.substr(pos, end - pos);
    }

    // 提取 creation_date
    pos = json_str.find("\"creation_date\": \"", pos);
    if (pos != std::string::npos) {
        pos += 18;
        size_t end = json_str.find("\"", pos);
        metadata.creation_date = json_str.substr(pos, end - pos);
    }

    // 提取 last_modified
    pos = json_str.find("\"last_modified\": \"", pos);
    if (pos != std::string::npos) {
        pos += 18;
        size_t end = json_str.find("\"", pos);
        metadata.last_modified = json_str.substr(pos, end - pos);
    }

    // 提取 version
    pos = json_str.find("\"version\": \"", pos);
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = json_str.find("\"", pos);
        metadata.version = json_str.substr(pos, end - pos);
    }

    auto project = std::make_shared<Project>(metadata);

    // TODO: 解析文件列表
    // 完整实现需要更复杂的 JSON 解析

    return project;
}

} // namespace DearTs::Core::ContentRegistry
