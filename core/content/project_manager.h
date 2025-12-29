/**
 * @file project_manager.h
 * @brief 项目管理器
 * @details 管理项目的创建、加载、保存和最近项目列表
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace DearTs::Core::ContentRegistry {

/**
 * @brief 项目元数据
 */
struct ProjectMetadata {
    std::string name;           ///< 项目名称
    std::string filepath;       ///< 项目文件路径
    std::string description;    ///< 项目描述
    std::string author;         ///< 作者
    std::string creation_date;  ///< 创建日期 (ISO 8601)
    std::string last_modified;  ///< 最后修改日期 (ISO 8601)
    std::string version;        ///< 项目版本
    std::vector<std::string> tags;  ///< 标签
    bool auto_save;             ///< 自动保存
    int auto_save_interval;     ///< 自动保存间隔（秒）

    /**
     * @brief 默认构造
     */
    ProjectMetadata()
        : name("Untitled")
        , description("")
        , author("")
        , creation_date("")
        , last_modified("")
        , version("1.0.0")
        , auto_save(true)
        , auto_save_interval(300) {}  // 默认 5 分钟
};

/**
 * @brief 项目文件
 */
struct ProjectFile {
    std::string path;           ///< 文件路径
    std::string type;           ///< 文件类型
    size_t size;                ///< 文件大小
    bool readonly;              ///< 是否只读

    ProjectFile()
        : size(0)
        , readonly(false) {}
};

/**
 * @brief 项目状态
 */
enum class ProjectState {
    Closed,        ///< 已关闭
    Opened,        ///< 已打开
    Modified,      ///< 已修改
    Saving,        ///< 保存中
    Loading        ///< 加载中
};

/**
 * @brief 项目
 */
class Project {
public:
    explicit Project(const ProjectMetadata& metadata);
    ~Project() = default;

    /**
     * @brief 获取项目元数据
     */
    [[nodiscard]] const ProjectMetadata& getMetadata() const { return m_metadata; }

    /**
     * @brief 获取项目元数据（可修改）
     */
    [[nodiscard]] ProjectMetadata& getMetadata() { return m_metadata; }

    /**
     * @brief 获取项目状态
     */
    [[nodiscard]] ProjectState getState() const { return m_state; }

    /**
     * @brief 设置项目状态
     */
    void setState(ProjectState state) { m_state = state; }

    /**
     * @brief 添加文件到项目
     */
    void addFile(const ProjectFile& file);

    /**
     * @brief 移除文件
     */
    void removeFile(const std::string& path);

    /**
     * @brief 获取所有文件
     */
    [[nodiscard]] const std::vector<ProjectFile>& getFiles() const { return m_files; }

    /**
     * @brief 标记为已修改
     */
    void markModified() { m_state = ProjectState::Modified; }

    /**
     * @brief 是否已修改
     */
    [[nodiscard]] bool isModified() const {
        return m_state == ProjectState::Modified;
    }

    /**
     * @brief 是否有未保存的更改
     */
    [[nodiscard]] bool hasUnsavedChanges() const {
        return m_state == ProjectState::Modified;
    }

private:
    ProjectMetadata m_metadata;
    std::vector<ProjectFile> m_files;
    ProjectState m_state;
};

/**
 * @brief 项目管理器
 *
 * 管理项目的生命周期，包括创建、打开、保存、关闭等操作
 */
class ProjectManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ProjectManager& instance() {
        static ProjectManager inst;
        return inst;
    }

    /**
     * @brief 创建新项目
     * @param metadata 项目元数据
     * @return 项目指针
     */
    std::shared_ptr<Project> createProject(const ProjectMetadata& metadata);

    /**
     * @brief 打开项目
     * @param filepath 项目文件路径
     * @return 项目指针，失败返回 nullptr
     */
    std::shared_ptr<Project> openProject(const std::string& filepath);

    /**
     * @brief 保存项目
     * @param project 项目指针
     * @param filepath 保存路径（为空则使用原路径）
     * @return 成功返回 true
     */
    bool saveProject(std::shared_ptr<Project> project, const std::string& filepath = "");

    /**
     * @brief 关闭项目
     * @param project 项目指针
     */
    void closeProject(std::shared_ptr<Project> project);

    /**
     * @brief 获取当前项目
     */
    [[nodiscard]] std::shared_ptr<Project> getCurrentProject() const { return m_current_project; }

    /**
     * @brief 设置当前项目
     */
    void setCurrentProject(std::shared_ptr<Project> project) { m_current_project = project; }

    /**
     * @brief 获取最近打开的项目列表
     */
    [[nodiscard]] const std::vector<std::string>& getRecentProjects() const {
        return m_recent_projects;
    }

    /**
     * @brief 添加到最近项目列表
     */
    void addRecentProject(const std::string& filepath);

    /**
     * @brief 清除最近项目列表
     */
    void clearRecentProjects();

    /**
     * @brief 保存最近项目列表到配置文件
     */
    bool saveRecentProjects(const std::string& config_path = "");

    /**
     * @brief 从配置文件加载最近项目列表
     */
    bool loadRecentProjects(const std::string& config_path = "");

    /**
     * @brief 项目是否需要保存
     */
    [[nodiscard]] bool needsSave() const;

    /**
     * @brief 自动保存当前项目
     */
    void autoSave();

    /**
     * @brief 设置自动保存回调
     */
    void setAutoSaveCallback(std::function<void(std::shared_ptr<Project>)> callback) {
        m_auto_save_callback = std::move(callback);
    }

private:
    ProjectManager() = default;
    ~ProjectManager() = default;

    // 禁止拷贝
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    /**
     * @brief 生成 JSON 项目文件
     */
    std::string projectToJSON(std::shared_ptr<Project> project);

    /**
     * @brief 从 JSON 解析项目
     */
    std::shared_ptr<Project> projectFromJSON(const std::string& json_str);

private:
    std::shared_ptr<Project> m_current_project;
    std::vector<std::string> m_recent_projects;
    std::function<void(std::shared_ptr<Project>)> m_auto_save_callback;
};

} // namespace DearTs::Core::ContentRegistry
