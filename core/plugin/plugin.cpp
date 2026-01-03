/**
 * @file plugin.cpp
 * @brief 插件系统实现
 */

#include "core/plugin/plugin.h"
#include "core/plugin/plugin_loader.h"
#include "liblogger/logger.h"
#include <format>

namespace DearTs::Core::Plugin {

// ================ PluginInfo ================

bool PluginInfo::is_api_compatible(const std::string& current_api_version) const {
    // 简单版本比较
    return api_version == current_api_version;
}

// ================ PluginWrapper ================

PluginWrapper::PluginWrapper(std::unique_ptr<IPlugin, PluginDeleter> plugin)
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

// ================ DynamicPluginWrapper ================

DynamicPluginWrapper::DynamicPluginWrapper(
    IPlugin* plugin,
    DestroyPluginFunc destroy_func,
    std::unique_ptr<DynamicLibraryLoader> loader,
    std::string source_path
)
    : m_destroy_func(destroy_func)
    , m_loader(std::move(loader))
    , m_source_path(std::move(source_path))
{
    // 创建带有自定义 deleter 的 unique_ptr 并存储到基类成员
    m_plugin = std::unique_ptr<IPlugin, PluginDeleter>(plugin, PluginDeleter{destroy_func});

    auto info = m_plugin->get_info();
    LOG_INFO("Created dynamic plugin wrapper for: {} from {}", info.name, m_source_path);
}

DynamicPluginWrapper::~DynamicPluginWrapper() {
    unload();
}

void DynamicPluginWrapper::unload() {
    // 先调用基类的 unload（会调用插件的 on_unload）
    PluginWrapper::unload();

    // 然后卸载动态库
    if (m_loader) {
        LOG_INFO("Unloading dynamic library: {}", m_source_path);
        m_loader->unload();
        m_loader.reset();
    }
}

// ================ PluginManager ================

Result<void, std::string> PluginManager::add_builtin(std::unique_ptr<IPlugin> plugin) {
    auto info = plugin->get_info();

    // 转换为使用 PluginDeleter 的 unique_ptr
    std::unique_ptr<IPlugin, PluginDeleter> plugin_with_deleter(plugin.release(), PluginDeleter{nullptr});

    // 检查是否已存在
    if (m_plugins.find(info.name) != m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' already exists", info.name)
        );
    }

    auto wrapper = std::make_unique<PluginWrapper>(std::move(plugin_with_deleter));
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
    // 1. 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return Result<void, std::string>::err(
            std::format("File not found: {}", path.string())
        );
    }

    // 2. 检查文件扩展名
    std::string ext = path.extension().string();
    bool valid_extension = false;

    #ifdef _WIN32
        valid_extension = (ext == ".dll");
    #elif defined(__linux__)
        valid_extension = (ext == ".so");
    #elif defined(__APPLE__)
        valid_extension = (ext == ".dylib");
    #endif

    if (!valid_extension) {
        return Result<void, std::string>::err(
            std::format("Invalid plugin extension: {} (expected: {})",
                ext,
                #ifdef _WIN32
                    ".dll"
                #elif defined(__linux__)
                    ".so"
                #elif defined(__APPLE__)
                    ".dylib"
                #endif
            )
        );
    }

    LOG_INFO("Loading plugin from: {}", path.string());

    // 3. 加载动态库
    auto loader = DynamicLibraryLoader::create();
    auto load_result = loader->load(path);

    if (load_result.isErr()) {
        return Result<void, std::string>::err(
            std::format("Failed to load library: {}", load_result.error())
        );
    }

    // 4. 解析符号：dearts_create_plugin
    auto create_symbol = loader->get_symbol("dearts_create_plugin");
    if (create_symbol.isErr()) {
        return Result<void, std::string>::err(
            std::format("Plugin export 'dearts_create_plugin' not found: {}", create_symbol.error())
        );
    }

    // 5. 解析符号：dearts_destroy_plugin
    auto destroy_symbol = loader->get_symbol("dearts_destroy_plugin");
    if (destroy_symbol.isErr()) {
        return Result<void, std::string>::err(
            std::format("Plugin export 'dearts_destroy_plugin' not found: {}", destroy_symbol.error())
        );
    }

    // 6. 获取函数指针
    auto create_func = reinterpret_cast<CreatePluginFunc>(create_symbol.unwrap());
    auto destroy_func = reinterpret_cast<DestroyPluginFunc>(destroy_symbol.unwrap());

    // 7. 创建插件实例
    IPlugin* plugin_raw = create_func();
    if (!plugin_raw) {
        return Result<void, std::string>::err(
            "Failed to create plugin instance (create_plugin returned null)"
        );
    }

    // 8. 包装为 unique_ptr（使用自定义 deleter）
    std::unique_ptr<IPlugin, DestroyPluginFunc> plugin(plugin_raw, destroy_func);

    // 9. 验证插件信息
    auto info = plugin->get_info();
    if (info.name.empty()) {
        return Result<void, std::string>::err("Plugin name cannot be empty");
    }

    // 10. 检查 API 版本兼容性
    const std::string current_api_version = "1.0.0";
    if (!info.is_api_compatible(current_api_version)) {
        return Result<void, std::string>::err(
            std::format("Plugin API version mismatch: plugin requires {}, current is {}",
                info.api_version, current_api_version)
        );
    }

    // 11. 检查是否已存在
    if (m_plugins.find(info.name) != m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' already exists", info.name)
        );
    }

    // 12. 创建包装器并加载
    auto wrapper = std::make_unique<DynamicPluginWrapper>(
        plugin_raw,
        destroy_func,
        std::move(loader),
        path.string()
    );

    auto wrapper_load_result = wrapper->load();
    if (wrapper_load_result.isErr()) {
        return Result<void, std::string>::err(
            std::format("Failed to load plugin '{}': {}", info.name, wrapper_load_result.error())
        );
    }

    wrapper->enable();
    m_plugins[info.name] = std::move(wrapper);

    LOG_INFO("Successfully loaded plugin: {} from {}", info.name, path.filename().string());
    return Result<void, std::string>::ok();
}

Result<size_t, std::string> PluginManager::load_from_directory(const std::filesystem::path& directory) {
    // 1. 检查目录是否存在
    if (!std::filesystem::exists(directory)) {
        return Result<size_t, std::string>::err(
            std::format("Directory '{}' does not exist", directory.string())
        );
    }

    // 2. 检查是否为目录
    if (!std::filesystem::is_directory(directory)) {
        return Result<size_t, std::string>::err(
            std::format("Path '{}' is not a directory", directory.string())
        );
    }

    LOG_INFO("Scanning directory for plugins: {}", directory.string());

    size_t count = 0;
    size_t failed = 0;
    std::vector<std::string> errors;

    // 3. 遍历目录（非递归）
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        // 4. 检查文件扩展名（过滤非插件文件）
        std::string ext = entry.path().extension().string();
        bool is_plugin = false;

        #ifdef _WIN32
            is_plugin = (ext == ".dll");
        #elif defined(__linux__)
            is_plugin = (ext == ".so");
        #elif defined(__APPLE__)
            is_plugin = (ext == ".dylib");
        #endif

        if (!is_plugin) {
            continue;
        }

        // 5. 尝试加载插件
        auto result = load_from_file(entry.path());

        if (result.isOk()) {
            count++;
            LOG_INFO("Successfully loaded: {}", entry.path().filename().string());
        } else {
            failed++;
            std::string error = result.error();
            errors.push_back(std::format("{}: {}", entry.path().filename().string(), error));
            LOG_WARN("Failed to load plugin: {} - {}", entry.path().filename().string(), error);
        }
    }

    // 6. 汇总日志
    LOG_INFO("Directory scan complete: {} loaded, {} failed", count, failed);

    if (!errors.empty() && failed > 0) {
        LOG_WARN("Failed plugins:");
        for (const auto& err : errors) {
            LOG_WARN("  - {}", err);
        }
    }

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
