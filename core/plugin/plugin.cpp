/**
 * @file plugin.cpp
 * @brief 插件系统实现
 */

#include "core/plugin/plugin.h"
#include "liblogger/logger.h"

namespace DearTs::Core::Plugin {

// ================ PluginInfo ================

bool PluginInfo::is_api_compatible(const std::string& current_api_version) const {
    // 简单版本比较
    return api_version == current_api_version;
}

// ================ PluginWrapper ================

PluginWrapper::PluginWrapper(std::unique_ptr<IPlugin> plugin)
    : m_plugin(std::move(plugin))
    , m_state(PluginState::Unloaded) {
    auto info = m_plugin->get_info();
    LOG_INFO("Created plugin wrapper for: {} v{} by {}",
             info.name, info.version, info.author);
}

PluginWrapper::~PluginWrapper() {
    if (m_state == PluginState::Enabled || m_state == PluginState::Loaded) {
        unload();
    }
}

PluginWrapper::PluginWrapper(PluginWrapper&& other) noexcept
    : m_plugin(std::move(other.m_plugin))
    , m_state(other.m_state)
    , m_error(std::move(other.m_error)) {
    other.m_state = PluginState::Unloaded;
}

PluginWrapper& PluginWrapper::operator=(PluginWrapper&& other) noexcept {
    if (this != &other) {
        if (m_state == PluginState::Enabled || m_state == PluginState::Loaded) {
            unload();
        }
        m_plugin = std::move(other.m_plugin);
        m_state = other.m_state;
        m_error = std::move(other.m_error);
        other.m_state = PluginState::Unloaded;
    }
    return *this;
}

Result<void, std::string> PluginWrapper::load() {
    if (m_state != PluginState::Unloaded) {
        return Result<void, std::string>::err("Plugin already loaded");
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Loading plugin: {}", info.name);

    auto result = m_plugin->on_load();
    if (result.isErr()) {
        m_error = result.error();
        m_state = PluginState::Error;
        LOG_ERROR("Failed to load plugin '{}': {}", info.name, m_error);
        return result;
    }

    m_state = PluginState::Loaded;
    LOG_INFO("Plugin loaded successfully: {}", info.name);
    return Result<void, std::string>::ok();
}

void PluginWrapper::unload() {
    if (m_state == PluginState::Enabled) {
        disable();
    }

    if (m_state == PluginState::Loaded) {
        auto info = m_plugin->get_info();
        LOG_INFO("Unloading plugin: {}", info.name);
        m_plugin->on_unload();
        m_state = PluginState::Unloaded;
    }
}

void PluginWrapper::enable() {
    if (m_state != PluginState::Loaded) {
        LOG_WARN("Cannot enable plugin in state: {}", static_cast<int>(m_state));
        return;
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Enabling plugin: {}", info.name);
    m_plugin->on_enable();
    m_state = PluginState::Enabled;
}

void PluginWrapper::disable() {
    if (m_state != PluginState::Enabled) {
        LOG_WARN("Cannot disable plugin in state: {}", static_cast<int>(m_state));
        return;
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Disabling plugin: {}", info.name);
    m_plugin->on_disable();
    m_state = PluginState::Loaded;
}

// ================ PluginManager ================

PluginManager& PluginManager::instance() {
    static PluginManager instance;
    return instance;
}

Result<void, std::string> PluginManager::add_builtin(std::unique_ptr<IPlugin> plugin) {
    auto info = plugin->get_info();

    // 检查是否已存在
    if (m_plugins.find(info.name) != m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' already exists", info.name)
        );
    }

    auto wrapper = std::make_unique<PluginWrapper>(std::move(plugin));
    auto result = wrapper->load();

    if (result.isErr()) {
        return result;
    }

    wrapper->enable();
    m_plugins[info.name] = std::move(wrapper);

    LOG_INFO("Added builtin plugin: {}", info.name);
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::load_from_file(const std::filesystem::path& path) {
    // TODO: 实现动态库加载
    LOG_INFO("Loading plugin from: {}", path.string());
    return Result<void, std::string>::err("Dynamic loading not implemented yet");
}

Result<size_t, std::string> PluginManager::load_from_directory(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return Result<size_t, std::string>::err(
            std::format("Directory '{}' does not exist", directory.string())
        );
    }

    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto result = load_from_file(entry.path());
            if (result.isOk()) {
                count++;
            }
        }
    }

    LOG_INFO("Loaded {} plugins from directory: {}", count, directory.string());
    return Result<size_t, std::string>::ok(count);
}

bool PluginManager::unload(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        LOG_WARN("Plugin '{}' not found", name);
        return false;
    }

    it->second->unload();
    m_plugins.erase(it);
    LOG_INFO("Unloaded plugin: {}", name);
    return true;
}

Result<void, std::string> PluginManager::enable(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }

    it->second->enable();
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::disable(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }

    it->second->disable();
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::reload(const std::string& name) {
    // TODO: 实现重载
    return Result<void, std::string>::err("Reload not implemented yet");
}

IPlugin* PluginManager::get_plugin(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        return it->second->get();
    }
    return nullptr;
}

std::vector<PluginInfo> PluginManager::get_all_plugins_info() const {
    std::vector<PluginInfo> infos;
    infos.reserve(m_plugins.size());

    for (const auto& [name, wrapper] : m_plugins) {
        auto* plugin = wrapper->get();
        if (plugin) {
            infos.push_back(plugin->get_info());
        }
    }

    return infos;
}

Result<PluginState, std::string> PluginManager::get_plugin_state(const std::string& name) const {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<PluginState, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }
    return Result<PluginState, std::string>::ok(it->second->get_state());
}

void PluginManager::clear() {
    LOG_INFO("Clearing all plugins...");
    m_plugins.clear();
}

} // namespace DearTs::Core::Plugin
